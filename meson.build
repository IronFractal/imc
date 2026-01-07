# @file meson.build
# @author James Hatfield
# @date 2025-11-11
# SPDX-License-Identifier: MIT
project(
    'imc',
    'c',
    default_options: [
        'werror=true',
        'c_std=gnu23',
        'warning_level=3',
    ],
    meson_version: '>=1.1.0',
)

imc_opts = []
imc_deps = []

stb_dep = subproject(
    'stb',
).get_variable('stb_dep')

stc_dep = subproject(
    'stc',
    default_options: [
        'tests=disabled',
        'examples=disabled',
        'warning_level=0',
    ],
).get_variable('stc_dep')

luajit_dep = subproject(
    'luajit',
    default_options: [
        'luajit=false',
        'warning_level=0',
        'default_library=static',
    ],
).get_variable('luajit_dep')

plutovg_dep = subproject(
    'plutovg',
    default_options: [
        'examples=disabled',
        'tests=disabled',
        'warning_level=0',
        'default_library=static',
    ],
).get_variable('plutovg_dep')

imc_deps += [
    stb_dep,
    stc_dep,
    luajit_dep,
    plutovg_dep,
]

if get_option('enable_asan')
    imc_opts += ['b_sanitize=address']
endif

imc_srcs = files([
    'src/xpm.c',
    'src/main.c',
    'src/langvm.c',
    'src/imagelib.c',
    'src/arg_parse.c',
    'src/stb_image_write_impl.c',
])

executable(
    meson.project_name(),
    imc_srcs,
    dependencies: imc_deps,
    override_options: imc_opts,
    include_directories: include_directories('include'),
)
