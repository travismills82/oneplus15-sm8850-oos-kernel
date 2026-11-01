#!/usr/bin/env python3
"""Fail-closed load-policy tests, not physical GEAS qualification."""
import importlib.util
from pathlib import Path
import unittest
import re
from test_coloros17_unified_kernel_backports import host_c

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('no_geas', HERE / 'prepare-coloros17-no-geas.py')
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class NoGEASPolicy(unittest.TestCase):
    def test_exact_names(self):
        self.assertEqual(policy.DISABLED, {'oplus_bsp_geas_cpu', 'oplus_bsp_geas_system', 'oplus_bsp_geas_data'})

    def test_merge_preserves_other_parameters(self):
        line = policy.command_line('console=test module_blacklist=old_provider foo=bar')
        self.assertIn('console=test foo=bar', line)
        self.assertIn('old_provider', line)
        self.assertEqual(line.count('module_blacklist='), 1)

    def test_duplicate_blacklist_rejected(self):
        with self.assertRaises(ValueError):
            policy.command_line('module_blacklist=a module_blacklist=b')

    def test_load_filter_exact_match(self):
        self.assertEqual(policy.filter_metadata('oplus_bsp_geas_data.ko\noplus_bsp_geas_data_test.ko\n', 'modules.load'),
                         'oplus_bsp_geas_data_test.ko\n')

    def test_remove_only_excluded_dependency_owner(self):
        original = '/vendor/lib/modules/oplus_bsp_geas_cpu.ko: game_opt.ko\nother.ko: game_opt.ko\n'
        self.assertEqual(policy.filter_metadata(original, 'modules.dep'), 'other.ko: game_opt.ko\n')

    def test_reverse_dependency_is_fatal(self):
        with self.assertRaises(ValueError):
            policy.filter_metadata('other.ko: /vendor/lib/modules/oplus_bsp_geas_cpu.ko\n', 'modules.dep')

    def test_unreviewed_alias_is_fatal(self):
        with self.assertRaises(ValueError):
            policy.filter_metadata('alias test oplus_bsp_geas_cpu\n', 'modules.alias')

    def test_protections_untouched(self):
        root = HERE.parents[1]
        source = (root / 'kernel_platform/common/kernel/module/main.c').read_text()
        self.assertIn('core_param(module_blacklist, module_blacklist, charp, 0400)', source)
        self.assertIn('if (blacklisted(info->name))', source)
        self.assertNotIn('oplus_bsp_geas', source)
        self.assertNotIn('get_gpu_bw_data', (root / 'vendor/qcom/opensource/graphics-kernel/kgsl_bus.c').read_text())

    def test_actual_kernel_blacklist_function_exact_matching(self):
        source = (HERE.parents[1] / 'kernel_platform/common/kernel/module/main.c').read_text()
        function = re.search(r'static bool blacklisted\(const char \*module_name\)\n\{.*?\n\}', source, re.S).group(0)
        program = '#include <assert.h>\n#include <stdbool.h>\n#include <string.h>\nstatic char *module_blacklist;\n' + function + r'''
int main(void) {
 assert(!blacklisted("oplus_bsp_geas_cpu"));
 module_blacklist="oplus_bsp_geas_cpu,oplus_bsp_geas_data,oplus_bsp_geas_system";
 assert(blacklisted("oplus_bsp_geas_cpu"));
 assert(blacklisted("oplus_bsp_geas_data"));
 assert(blacklisted("oplus_bsp_geas_system"));
 assert(!blacklisted("oplus_bsp_geas_cpu_test"));
 assert(!blacklisted("oplus_bsp_geas"));
 assert(!blacklisted("msm_kgsl"));
 assert(!blacklisted("oplus_bsp_game_opt"));
 return 0;
}
'''
        result = host_c(program)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
