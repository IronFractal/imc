# @file meson.build
# @author James Hatfield
# @date 2025-11-11
# SPDX-License-Identifier: MIT
project(
    'imc',
    'c',
    default_options: [
        'werror=true',
        'c_std=c23',
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

subdir('libimc')

imc_deps += [
    stc_dep,
    libimc_dep,
]

imc_srcs = files([
    'src/main.c',
    'src/arg_parse.c',
    'src/parse_stream.c',
])

executable(
    meson.project_name(),
    imc_srcs,
    dependencies: imc_deps,
    include_directories: include_directories('include'),
)
