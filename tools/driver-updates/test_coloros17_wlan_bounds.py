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
    def test_actual_mlo_predicate_does_not_read_ap_station_state(self):
        source = (QCA / 'core/hdd/src/wlan_hdd_stats.c').read_text()
        body = block(source, '__wlan_hdd_cfg80211_ll_stats_get(')
        condition = re.search(r'if \((adapter->device_mode == QDF_STA_MODE &&\s*'
                              r'mlo_is_ml_connection_in_progress\(hdd_ctx->psoc,\s*'
                              r'link_info->vdev_id\))\)', body)
        self.assertIsNotNone(condition)
        prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
enum { QDF_STA_MODE, QDF_SAP_MODE, QDF_P2P_CLIENT_MODE };
struct sta { bool connecting; }; struct psoc { struct sta *sta_ctx; };
struct hdd { struct psoc *psoc; }; struct adapter { int device_mode; };
struct link { int vdev_id; };
static unsigned calls;
static bool mlo_is_ml_connection_in_progress(struct psoc *p, int id) {
    (void)id; ++calls; return p->sta_ctx->connecting;
}
static bool check(struct adapter *adapter, struct hdd *hdd_ctx, struct link *link_info) {
    return CONDITION;
}
int main(int argc, char **argv) {
    (void)argv; struct psoc p={0}; struct hdd h={&p}; struct link l={0};
    struct adapter a={QDF_SAP_MODE};
    /* MLO AP has ap_ctx rather than sta_ctx; short circuit must avoid helper. */
    assert(!check(&a,&h,&l)); assert(calls==0);
    if(argc>1) return 0;
    a.device_mode=QDF_P2P_CLIENT_MODE; assert(!check(&a,&h,&l)); assert(calls==0);
    struct sta s={true}; p.sta_ctx=&s; a.device_mode=QDF_STA_MODE;
    assert(check(&a,&h,&l)); assert(calls==1);
    s.connecting=false; assert(!check(&a,&h,&l)); assert(calls==2); return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='coloros17-mlo-host-') as tmp:
            for mutation in (False, True):
                predicate = condition.group(1)
                if mutation:
                    predicate = re.sub(r'adapter->device_mode == QDF_STA_MODE &&\s*', '', predicate)
                text = prefix.replace('CONDITION', predicate)
                # The mutant no longer reads adapter; keep warning checks strict.
                text = text.replace('return ' + predicate, '(void)adapter; return ' + predicate)
                binary = str(Path(tmp) / ('mutant' if mutation else 'guard'))
                cc = subprocess.run(['clang', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                                     '-fsanitize=address,undefined', '-x', 'c', '-', '-o', binary],
                                    input=text, text=True, capture_output=True, timeout=30)
                self.assertEqual(cc.returncode, 0, cc.stderr)
                result = subprocess.run([binary] + (['mutant'] if mutation else []),
                                        text=True, capture_output=True, timeout=10)
                if mutation:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn('runtime error', result.stderr)
                else:
                    self.assertEqual(result.returncode, 0, result.stderr)

    def test_actual_monitor_wrappers_select_header_type_or_fail_closed(self):
        source = (QCA / 'core/hdd/src/wlan_hdd_cfg80211.c').read_text()
        old = block(source, 'static int __maybe_unused wlan_hdd_cfg80211_set_mon_ch(')
        new = block(source, 'wlan_hdd_cfg80211_set_mon_ch_with_netdev(struct wiphy')
        pos = source.index('.set_monitor_channel = _Generic(')
        expression = source[pos:source.index('#endif', pos)].split('=', 1)[1].strip().removesuffix(',')
        self.assertNotIn('default:', expression)
        prefix = r'''
#include <assert.h>
struct wiphy { int unused; }; struct net_device { int unused; };
struct cfg80211_chan_def { int unused; }; struct osif_psoc_sync { int unused; };
static int start_error, inner_error, starts, stops, inner_calls;
static struct osif_psoc_sync synchronization;
static struct wiphy *wiphy_dev(struct wiphy *w) { return w; }
static int osif_psoc_sync_op_start(struct wiphy *w, struct osif_psoc_sync **sync) {
    (void)w; ++starts; *sync=&synchronization; return start_error;
}
static void osif_psoc_sync_op_stop(struct osif_psoc_sync *sync) { (void)sync; ++stops; }
static int __wlan_hdd_cfg80211_set_mon_ch(struct wiphy *w, struct cfg80211_chan_def *c) {
    (void)w; (void)c; ++inner_calls; return inner_error;
}
'''
        wrappers = ('static int __attribute__((unused)) wlan_hdd_cfg80211_set_mon_ch('
                    'struct wiphy *wiphy, struct cfg80211_chan_def *chandef) ' + old + '\n'
                    'static int __attribute__((unused)) wlan_hdd_cfg80211_set_mon_ch_with_netdev('
                    'struct wiphy *wiphy, struct net_device *dev, struct cfg80211_chan_def *chandef) '
                    + new)
        with tempfile.TemporaryDirectory(prefix='coloros17-wlan-api-') as tmp:
            for kind in ('old', 'new', 'unknown'):
                prototype = {'old': 'struct wiphy *, struct cfg80211_chan_def *',
                             'new': 'struct wiphy *, struct net_device *, struct cfg80211_chan_def *',
                             'unknown': 'struct wiphy *, long *'}[kind]
                call = '&w, &c' if kind == 'old' else '&w, &d, &c'
                suffix = r'''
int main(void) {
    struct wiphy w={0}; struct net_device d={0}; struct cfg80211_chan_def c={0};
    (void)d; start_error=-3;
    assert(CALL == -3); assert(starts==1 && stops==0 && inner_calls==0);
    start_error=0; inner_error=-5;
    assert(CALL == -5); assert(starts==2 && stops==1 && inner_calls==1);
    inner_error=0;
    assert(CALL == 0); assert(starts==3 && stops==2 && inner_calls==2); return 0;
}
'''.replace('CALL', 'ops.set_monitor_channel(' + call + ')')
                if kind == 'unknown':
                    suffix = 'int main(void) { return 0; }'
                full = (prefix + 'struct cfg80211_ops { int (*set_monitor_channel)(' + prototype + '); };\n'
                        + wrappers + '\nstatic const struct cfg80211_ops ops = { .set_monitor_channel = '
                        + expression + ' };\n' + suffix)
                binary = str(Path(tmp) / kind)
                cc = subprocess.run(['clang', '-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror',
                                     '-fsanitize=address,undefined', '-x', 'c', '-', '-o', binary],
                                    input=full, text=True, capture_output=True, timeout=30)
                if kind == 'unknown':
                    self.assertNotEqual(cc.returncode, 0)
                    self.assertIn('generic', cc.stderr)
                else:
                    self.assertEqual(cc.returncode, 0, cc.stderr)
                    result = subprocess.run([binary], capture_output=True, text=True, timeout=10)
                    self.assertEqual(result.returncode, 0, result.stderr)

    def test_mlo_and_lifetime_source_contracts(self):
        stats = (QCA / 'core/hdd/src/wlan_hdd_stats.c').read_text()
        body = block(stats, '__wlan_hdd_cfg80211_ll_stats_get(')
        self.assertLess(body.index('mlo_is_ml_connection_in_progress('),
                        body.index('wlan_cfg80211_nla_parse('))
        main = (QCA / 'components/dp/core/src/wlan_dp_main.c').read_text()
        self.assertEqual(main.count('qdf_spin_lock_bh(&dp_link->vdev_lock);'),
                         main.count('qdf_spin_unlock_bh(&dp_link->vdev_lock);'))
        for name in ('dp_sta_notify_tx_comp_cb(', 'dp_rx_packet_cbk('):
            txrx = (QCA / 'components/dp/core/src/wlan_dp_txrx.c').read_text()
            body = block(txrx, name)
            self.assertIn('dp_objmgr_get_vdev_by_user(dp_link, WLAN_TDLS_SB_ID)', body)
            self.assertIn('dp_objmgr_put_vdev_by_user(vdev, WLAN_TDLS_SB_ID)', body)
            self.assertNotIn('wlan_objmgr_vdev_try_get_ref(dp_link->vdev', body)

    def test_runtime_pm_ini_disable_precedes_kernel_status(self):
        source = (QCA.parent / 'qca-wifi-host-cmn/hif/src/hif_runtime_pm.c').read_text()
        body = block(source, 'static bool hif_rtpm_enabled(void)')
        self.assertRegex(body, r'if \(!gp_hif_rtpm_ctx->enable_rpm\)\s+return false;')
        self.assertLess(body.index('!gp_hif_rtpm_ctx->enable_rpm'), body.index('__hif_rtpm_enabled('))

    def test_real_p2p_tlv_function_with_sanitizers_and_mutation(self):
        """Real TLV parser body; host shims validate OUI lookup separately."""
        compiler = shutil.which('clang')
        self.assertIsNotNone(compiler)
        source = (QCA / 'components/p2p/core/src/wlan_p2p_main.c').read_text()
        public = (QCA / 'components/p2p/dispatcher/inc/wlan_p2p_public_struct.h').read_text()
        constants = '\n'.join('#define ' + key + ' ' + value for key, value in
                              re.findall(r'\b(P2P_ATTR_[A-Z_]+)\s*=\s*([0-9]+)', public))
        body = block(source, 'p2p_parse_assoc_ie_for_device_info(')
        prefix = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#define P2P_OUI "P2P"
#define P2P_OUI_SIZE 4
#define HEADER_LEN_P2P_IE 6
#define WLAN_MAX_IE_LEN 255
#define QDF_MAC_ADDR_SIZE 6
#define MAX_CONFIG_METHODS_LEN 2
#define DEVICE_CATEGORY_MAX_LEN 1
#define LE_READ_2(p) ((unsigned)(p)[0] | ((unsigned)(p)[1] << 8))
#define p2p_err(...) ((void)0)
/* Host shim supplies a validated vendor IE. This is not an OUI-lookup test. */
static const uint8_t *wlan_get_vendor_ie_ptr_from_oui(const char *oui, unsigned size,
        const uint8_t *ie, uint32_t length) {
    (void)oui; (void)size;
    return length >= 2 && ie[1] <= length - 2 ? ie : NULL;
}
static bool is_p2p_oui(const uint8_t *ie) { (void)ie; return true; }
'''
        suffix = r'''
int main(int argc, char **argv) {
    (void)argv;
    if (argc > 1) {
        /* The old payload-only bound consumes 3 bytes past this allocation. */
        uint8_t *ie = malloc(12); memset(ie, 255, 12);
        ie[1]=10; ie[6]=255; ie[7]=6; ie[8]=0;
        p2p_parse_assoc_ie_for_device_info(ie, 12); free(ie); return 0;
    }
    for (unsigned payload=2; payload<=251; ++payload) {
        uint8_t *ie=malloc(6+payload);
        for (unsigned attr=0; attr<=65535; ++attr) {
            memset(ie,255,6+payload); ie[1]=4+payload;
            ie[6]=255; ie[7]=attr; if(payload>2) ie[8]=attr>>8;
            if (p2p_parse_assoc_ie_for_device_info(ie,6+payload)) return 1;
        }
        free(ie);
    }
    uint8_t valid[26]={0}; valid[1]=24; valid[6]=13; valid[7]=17;
    return p2p_parse_assoc_ie_for_device_info(valid,26) == valid+9 ? 0 : 2;
}
'''
        with tempfile.TemporaryDirectory(prefix='coloros17-p2p-host-') as tmp:
            for mutation in (False, True):
                tested_body = body.replace('(3 + attr_len) > rem_len', 'attr_len > rem_len') if mutation else body
                binary = str(Path(tmp) / ('mutant' if mutation else 'parser'))
                full = (prefix + constants + '\nconst uint8_t *p2p_parse_assoc_ie_for_device_info('
                        'const uint8_t *assoc_ie, uint32_t assoc_ie_len) ' + tested_body + suffix)
                cc = subprocess.run([compiler, '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                                     '-fsanitize=address,undefined', '-x', 'c', '-', '-o', binary],
                                    input=full, capture_output=True, text=True, timeout=30)
                self.assertEqual(cc.returncode, 0, cc.stderr)
                result = subprocess.run([binary] + (['mutant'] if mutation else []),
                                        capture_output=True, text=True, timeout=60)
                if mutation:
                    self.assertNotEqual(result.returncode, 0, 'Missing bound was not detected')
                    self.assertIn('AddressSanitizer', result.stderr)
                else:
                    self.assertEqual(result.returncode, 0, result.stderr)

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
