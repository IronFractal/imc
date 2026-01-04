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
)

imc_deps = []

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
    stc_dep,
    luajit_dep,
    plutovg_dep,
]

imc_srcs = files([
    'src/main.c',
    'src/langvm.c',
    'src/imagelib.c',
    'src/arg_parse.c',
])

executable(
    meson.project_name(),
    imc_srcs,
    dependencies: imc_deps,
    include_directories: include_directories('include'),
)
