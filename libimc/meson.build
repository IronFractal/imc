libimc_srcs = files([
    'src/lexer.c',
])

libimc_incl = include_directories('include')

libimc = library(
    'libimc',
    libimc_srcs,
    dependencies: [stc_dep],
    include_directories: libimc_incl,
)

libimc_dep = declare_dependency(
    link_with: libimc,
    include_directories: libimc_incl,
)
