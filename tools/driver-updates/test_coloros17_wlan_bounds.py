#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host scalar/source tests; not a WLAN module, firmware or device test."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
QCA = ROOT / 'vendor/qcom/opensource/wlan/qcacld-3.0'


def block(source, start):
    pos = source.index('{', source.index(start))
    end, depth = pos + 1, 1
    while depth:
        if end == len(source):
            raise ValueError('Unterminated source block')
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[pos:end]


def function(source, name):
    return block(source, 'static void ' + name + '(')


class WlanBounds(unittest.TestCase):
    def test_reassociation_span_and_short_frame_guard(self):
        source = (QCA / 'core/mac/src/sys/legacy/src/utils/src/parser_api.c').read_text()
        body = block(source, 'sir_convert_reassoc_req_frame2_mlo_struct(')
        self.assertLess(body.index('nframe < WLAN_REASSOC_REQ_IES_OFFSET'),
                        body.index('util_find_mlie('))
        self.assertRegex(body, r' util_find_mlie\(pframe \+ WLAN_REASSOC_REQ_IES_OFFSET,\s*'
                              r'nframe - WLAN_REASSOC_REQ_IES_OFFSET,')
        for length in range(65536):
            if length >= 10:
                self.assertEqual(10 + (length - 10), length)
                self.assertGreater(10 + (length - 4), length)

    def test_real_rate_guards_precede_both_table_reads(self):
        source = (QCA / 'core/hdd/src/wlan_hdd_stats.c').read_text()
        for mode in ('he', 'vht'):
            body = function(source, 'hdd_get_max_rate_' + mode)
            guard = 'if (mcsidx >= ARRAY_SIZE(supported_' + mode + '_mcs_rate_nss1))'
            self.assertEqual(body.count(guard), 2)
            self.assertLess(body.index(guard), body.index('for (i = 0;'))
            self.assertLess(body.rindex(guard), body.index('supported_' + mode + '_mcs_rate[mcsidx]'))
            self.assertIn('i < MAX_RSSI_MCS_INDEX', body)
            self.assertNotIn('mcsidx > MAX_HT_MCS_INDEX', body)

    def test_actual_rate_functions_under_host_sanitizers(self):
        """Compile real functions/tables with minimal scalar host-only shims."""
        compiler = shutil.which('cc')
        self.assertIsNotNone(compiler, 'Host compiler is required for this test')
        source = (QCA / 'core/hdd/src/wlan_hdd_stats.c').read_text()
        header = (QCA / 'core/hdd/src/wlan_hdd_stats.h').read_text()
        chunks = [r'''
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BUILD_BUG_ON(x) _Static_assert(!(x), "source table invariant")
#define MAX_RSSI_MCS_INDEX 14
#define INVALID_MCS_IDX 255
#define DATA_RATE_11AX_MCS_MASK 3
#define DATA_RATE_11AC_MCS_MASK 3
#define MAX_HE_DCM_INDEX 2
#define hdd_err(...) ((void)0)
#define hdd_debug(...) ((void)0)
enum data_rate_11ax_max_mcs { DATA_RATE_11AX_MAX_MCS_9,
    DATA_RATE_11AX_MAX_MCS_10, DATA_RATE_11AX_MAX_MCS_11, DATA_RATE_11AX_MAX_MCS_NA };
enum data_rate_11ac_max_mcs { DATA_RATE_11AC_MAX_MCS_7,
    DATA_RATE_11AC_MAX_MCS_8, DATA_RATE_11AC_MAX_MCS_9, DATA_RATE_11AC_MAX_MCS_NA };
/* Host-only field shims: no kernel/private ABI claim is made. */
struct hdd_station_info { unsigned tx_mcs_map, he_mcs_12_13_map, vht_mcs_10_11_supp; };
struct hdd_fw_txrx_stats { struct { uint8_t mcs; } tx_rate; int8_t rssi; };
struct index_vht_data_rate_type { uint8_t beacon_rate_index;
    uint16_t supported_VHT80_rate[2], supported_VHT40_rate[2], supported_VHT20_rate[2]; };
enum { TX_RATE_HE20=1, TX_RATE_HE40=2, TX_RATE_HE80=4, TX_RATE_HE160=8,
       TX_RATE_VHT20=16, TX_RATE_VHT40=32, TX_RATE_VHT80=64, TX_RATE_SGI=128 };
''']
        for kind in ('supported_vht_mcs_rate_nss1', 'supported_vht_mcs_rate_nss2'):
            chunks.append('static const struct index_vht_data_rate_type ' + kind + '[] = ' +
                          block(source, kind + '[] =') + ';')
        chunks.append('struct index_he_data_rate_type ' +
                      block(header, 'struct index_he_data_rate_type {') + ';')
        for kind in ('supported_he_mcs_rate_nss1', 'supported_he_mcs_rate_nss2'):
            chunks.append('static const struct index_he_data_rate_type ' + kind + '[] = ' +
                          block(header, kind + '[] =') + ';')
        chunks.append('static int rssi_mcs_tbl[][MAX_RSSI_MCS_INDEX] = ' +
                      block(source, 'rssi_mcs_tbl[][MAX_RSSI_MCS_INDEX] =') + ';')
        signature = ('(struct hdd_station_info *stainfo, struct hdd_fw_txrx_stats *stats, '
                     'uint32_t rate_flags, uint8_t nss, uint32_t *maxrate, '
                     'uint8_t *max_mcs_idx, bool report_max) ')
        for mode in ('he', 'vht'):
            chunks.append('static void hdd_get_max_rate_' + mode + signature +
                          function(source, 'hdd_get_max_rate_' + mode))
        chunks.append(r'''
_Static_assert(ARRAY_SIZE(supported_he_mcs_rate_nss1) == 14, "HE count");
_Static_assert(ARRAY_SIZE(supported_vht_mcs_rate_nss1) == 12, "VHT count");
int main(void) {
    struct hdd_station_info station = {0};
    struct hdd_fw_txrx_stats stats = {0};
    const int8_t signal[] = {-128, -90, -42, 0, 127};
    for (unsigned nss=1; nss<=2; ++nss)
    for (unsigned mcs=0; mcs<256; ++mcs)
    for (unsigned cap=0; cap<4; ++cap)
    for (unsigned feature=0; feature<2; ++feature)
    for (unsigned report=0; report<2; ++report)
    for (unsigned r=0; r<ARRAY_SIZE(signal); ++r) {
        station.tx_mcs_map=cap; station.he_mcs_12_13_map=feature;
        station.vht_mcs_10_11_supp=feature;
        stats.tx_rate.mcs=mcs; stats.rssi=signal[r];
        for (unsigned b=0; b<4; ++b) for (unsigned gi=0; gi<2; ++gi) {
            uint32_t rate=0; uint8_t result=255;
            hdd_get_max_rate_he(&station, &stats, (1U<<b)|(gi?TX_RATE_SGI:0),
                               nss, &rate, &result, report);
            if (result>=14 || (report && feature && result!=13)) return 1;
        }
        for (unsigned b=0; b<3; ++b) for (unsigned gi=0; gi<2; ++gi) {
            uint32_t rate=0; uint8_t result=255;
            hdd_get_max_rate_vht(&station, &stats, (16U<<b)|(gi?TX_RATE_SGI:0),
                                nss, &rate, &result, report);
            if (result>=12 || (report && feature && result!=11)) return 2;
        }
    }
    return 0;
}
''')
        with tempfile.TemporaryDirectory(prefix='coloros17-wlan-host-') as temporary:
            binary = str(Path(temporary) / 'rates')
            compiled = subprocess.run([compiler, '-std=c11', '-O1', '-Wall', '-Wextra',
                                       '-Werror', '-fsanitize=address,undefined',
                                       '-fno-omit-frame-pointer', '-x', 'c', '-', '-o', binary],
                                      input='\n'.join(chunks), text=True, capture_output=True,
                                      timeout=30)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            tested = subprocess.run([binary], capture_output=True, text=True, timeout=60)
            self.assertEqual(tested.returncode, 0, tested.stderr)


if __name__ == '__main__':
    unittest.main()
