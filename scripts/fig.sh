#!/bin/bash
# MIT License
#
# Copyright (c) 2025 James Hatfield
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Description:
# A bash script for generating generic configuration scripts.
#
# Usage:
# Source from your "fig" script.
# ```
# #!/usr/bin/env fig.sh
# ```
# or
# ```
# #!/bin/bash
#
# if ! ${FIG_DEFINED:-false} ; then
#     exec "$(dirname "$(readlink -f "${0}")")/fig.sh" ${0} ${@}
# fi
# ```
#
# Git Repository: https://github.com/IronFractal/fig
#
set -e

FIG_DEFINED=true

__FIG_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
__FIG_SCRIPT_DIR="$(cd -- "$(dirname -- "${1}")" &> /dev/null && pwd)"
__FIG_SCRIPT="""#!/bin/bash
# THIS IS A GENERATED SCRIPT (DO NOT EDIT)!!
# Source: ${1}
set -e

SRC_DIR=\"\$(cd -- \"\$(dirname -- \"\${BASH_SOURCE[0]}\")\" &> /dev/null && pwd)\"
SRC_DIR_RELATIVE=\"\$(realpath --relative-to=\$(pwd) \"\${SRC_DIR}\")\"
BUILD_DIR=\"\$(pwd)\"
"""
__FIG_EXPORTED=""

__FIG_PARSER=""
__FIG_PARSER_SHORT=""
__FIG_PARSER_LONG=""
__FIG_PARSER_ENV=false
__FIG_PARSER_USAGE=true
__FIG_PARSER_DESC=""
__FIG_PARSER_HELP=""
__FIG_PARSER_HELP_ENV=""

fig_log() {
    echo "${1}"
}

fig_log_err() {
    echo "error: ${1}" >&2
}

fig_get_script_path() {
    if [[ " ${1} " =~ ^/.* ]] ; then
        echo "${1}"
    else
        echo "$(realpath ${__FIG_SCRIPT_DIR}/${1})"
    fi
}

fig_generate() {
    local SCRIPT_NAME="$(basename ${1})"
    local SCRIPT_PATH="$(fig_get_script_path ${1})"
    if [ -z "${SCRIPT_NAME}" ] ; then
        fig_log_err "configure script name cannot be empty!"
        return 1
    elif [ -e "${SCRIPT_PATH}" ] ; then
        fig_log_err "cannot overwrite existing file '${SCRIPT_PATH}'!"
        return 1
    fi
    fig_export main
    mkdir -p "$(dirname ${SCRIPT_PATH})"
    echo "${__FIG_SCRIPT}" > ${SCRIPT_PATH}
    echo "main \${@}" >> ${SCRIPT_PATH}
    echo "" >> ${SCRIPT_PATH}
    chmod +x ${SCRIPT_PATH}
}

fig_export() {
    if [[ " ${__FIG_EXPORTED} " =~ " ${1} " ]] ; then
        return 0
    fi
    if [ -z "$(type -t ${1})" ] ; then
        fig_log_err "function '${1}' does not exist!"
        return 1
    fi
    if [ "$(type -t ${1})" != "function" ] ; then
        fig_log_err "'${1}' is not a function!"
        return 1
    fi
    __FIG_EXPORTED+=" ${1} "
    __FIG_SCRIPT="""${__FIG_SCRIPT}
$(declare -f "${1}" | sed 's/[[:space:]]*$//')
"""
}

fig_assert() {
    if ! which ${1} &>/dev/null ; then
        fig_log_err "command '${1}' is not available!"
        return 1
    fi
    return 0
}

fig_assert_qq() {
    fig_assert "${@}" &>/dev/null
}

fig_assert_one() {
    local CHECKED=""
    for cmd in ${@} ; do
        if which ${cmd} &>/dev/null ; then
            echo "${cmd}"
            return 0
        fi
        CHECKED+=" '${cmd}' "
    done
    fig_log_err "one of${CHECKED}is not available!"
    return 1
}

fig_assert_one_q() {
    fig_assert_one "${@}" >/dev/null
}

fig_assert_one_qq() {
    fig_assert_one "${@}" &>/dev/null
}

fig_parser_begin() {
    fig_assert fold
    fig_assert getopt

    __FIG_PARSER="${1}"
    __FIG_PARSER_SHORT="h"
    __FIG_PARSER_LONG="help,"
    __FIG_PARSER_ENV=false
    __FIG_PARSER_ENV_DEFAULTS=""
    __FIG_PARSER_USAGE=true
    __FIG_PARSER_DESC=""
    __FIG_PARSER_PRE=""
    __FIG_PARSER_CASE=""
    __FIG_PARSER_HELP="""Options:
  -h, --help
    Print this help information
"""
    __FIG_PARSER_HELP_ENV="""
Environment:
"""
}

fig_parser_enable_usage() {
    if ${1} ; then
        __FIG_PARSER_USAGE=true
    else
        __FIG_PARSER_USAGE=false
    fi
}

fig_parser_set_description() {
    __FIG_PARSER_DESC="${1}"
}

fig_parser_add_opt() {
    if [ -z "${__FIG_PARSER}" ] ; then
        fig_log_err "no active parser, did you forget to call fig_parser_begin!"
        return 1
    fi
    local TYPE="${1}"
    local ARG="none"
    local VALUE=""
    local DEFAULT="$(echo "${2}" | tr '=' ' ' | awk '{print $2}')"
    local FLAGS="$(echo "${2}" | tr '=' ' ' | awk '{print $1}')"
    local SHORT="$(echo "${FLAGS}" | tr ',' ' ' | awk '{print $1}')"
    local LONG="$(echo "${FLAGS}" | tr ',' ' ' | awk '{print $2}')"
    local SHORT_US="$(echo "${SHORT}" | tr '-' '_')"
    local LONG_US="$(echo "${LONG}" | tr '-' '_')"
    local PRIMARY="${LONG_US}"
    local DESC="${3}"
    local CASE=""
    local SHIFT=""
    local HELP="  "
    local GETOPTARG=""

    if [ "$(echo -n "${SHORT}" | wc -c)" -gt 1 ] ; then
        if [ -n "${LONG}" ] ; then
            fig_log_err "invalid short option '${SHORT}' provided!"
            return 1
        fi
        LONG="${SHORT}"
        LONG_US="${SHORT_US}"
        PRIMARY="${SHORT_US}"
        SHORT=""
        SHORT_US=""
    fi

    if [ -z "${LONG_US}" ] ; then
        PRIMARY="${SHORT_US}"
    fi

    case "${TYPE}" in
        flag)
            VALUE="=true"
            DEFAULT="false"
            ;;
        option)
            ARG="required"
            VALUE="=\"\${2}\""
            DEFAULT="\"${DEFAULT:-""}\""
            ;;
        array)
            ARG="required"
            VALUE="+=\" \${2} \""
            DEFAULT="\"${DEFAULT:-""}\""
            ;;
        *)
            fig_log_err "unknown option type '${TYPE}'!"
            return 1
            ;;
    esac

    case "${ARG}" in
        none)
            ;;
        required)
            GETOPTARG=":"
            SHIFT=" 2"
            ;;
        optional)
            GETOPTARG="::"
            SHIFT=" 2"
            ;;
    esac

    if [ -n "${SHORT}" ] ; then
        __FIG_PARSER_SHORT+="${SHORT}${GETOPTARG}"
        CASE="-${SHORT}"
        HELP+="-${SHORT}"
    fi
    if [ -n "${LONG}" ] ; then
        __FIG_PARSER_LONG+="${LONG}${GETOPTARG},"
        if [ -n "${SHORT}" ] ; then
            CASE+="|"
            HELP+=", "
        fi
        CASE+="--${LONG}"
        HELP+="--${LONG}"
    fi

    if [ "${ARG}" == "required" ] ; then
        HELP+=" <argument>"
    elif [ "${ARG}" == "optional" ] ; then
        HELP+=" [argument]"
    fi

    __FIG_PARSER_HELP+="""${HELP}
$(echo "${DESC}" | fold -w 76 -s - | sed 's/[[:space:]]*$//' | sed 's/^/    /')
"""

    __FIG_PARSER_PRE+="""OPT_${__FIG_PARSER}_${PRIMARY}_set=false
OPT_${__FIG_PARSER}_${PRIMARY}=${DEFAULT}
"""

    __FIG_PARSER_CASE+="""${CASE})
    OPT_${__FIG_PARSER}_${PRIMARY}_set=true
    OPT_${__FIG_PARSER}_${PRIMARY}${VALUE}
    shift${SHIFT}
    ;;
"""
}

fig_parser_add_env() {
    local NAME="$(echo "${1}" | tr '=' ' ' | awk '{print $1}')"
    local DEFAULT="$(echo "${1}" | tr '=' ' ' | awk '{print $2}')"
    local DESC="${2}"
    local HELP="<value>"

    __FIG_PARSER_ENV=true

    if [ -n "${DEFAULT}" ] ; then
        HELP="[value] (default: \\\"${DEFAULT}\\\")"
        __FIG_PARSER_ENV_DEFAULTS+="""${NAME}=\"\${${NAME}:-\"${DEFAULT}\"}\"
"""
    fi

    __FIG_PARSER_HELP_ENV+="""  ${NAME}=${HELP}
$(echo "${DESC}" | fold -w 76 -s - | sed 's/[[:space:]]*$//' | sed 's/^/    /')
"""
}

fig_parser_end() {
     if [ -z "${__FIG_PARSER}" ] ; then
        fig_log_err "no active parser, did you forget to call fig_parser_begin!"
        return 1
    fi
    __FIG_SCRIPT+="""
print_help_${__FIG_PARSER} ()
{
    echo \"\"\""""
    if ${__FIG_PARSER_USAGE} ; then
        __FIG_SCRIPT+="""Usage:
  \${0} [options]

"""
    fi
    if [ -n "${__FIG_PARSER_DESC}" ] ; then
        __FIG_SCRIPT+="""    ${__FIG_PARSER_DESC}

"""
    fi
    __FIG_SCRIPT+="""$(echo "${__FIG_PARSER_HELP}" | sed 's/^\n$//')"""
    if ${__FIG_PARSER_ENV} ; then
        __FIG_SCRIPT+="""
$(echo "${__FIG_PARSER_HELP_ENV}" | sed 's/^\n$//')"""
    fi
    __FIG_SCRIPT+="""\"\"\"
}
"""
    __FIG_SCRIPT+="""
parse_opts_${__FIG_PARSER} ()
{
    local ERR=\$(getopt -Q -o ${__FIG_PARSER_SHORT} --long ${__FIG_PARSER_LONG} -- \"\$@\" 2>&1)
    local ARGS=\$(getopt -q -o ${__FIG_PARSER_SHORT} --long ${__FIG_PARSER_LONG} -- \"\$@\")
    eval set -- \"\${ARGS}\"

    if [ -n \"\${ERR}\" ] ; then
        fig_log_err \"\$(echo \"\${ERR}\" | sed 's/getopt: //')\"
        print_help_${__FIG_PARSER}
        return 1
    fi

$(echo "${__FIG_PARSER_PRE}" | sed 's/^/    /' | sed 's/^[[:space:]]*$//' )

    while true ; do
        case \"\$1\" in
            -h|--help)
                print_help_${__FIG_PARSER}
                exit 0
                ;;
$(echo "${__FIG_PARSER_CASE}" | sed 's/^/            /' | sed 's/^[[:space:]]*$//')
            --)
                shift
                break
                ;;
            *)
                fig_log_err \"unknown argument!\"
                print_help_${__FIG_PARSER}
                return 1
                ;;
        esac
    done

    if [ -n \"\${1}\" ] ; then
        fig_log_err \"unhandled positional arguments!\"
        return 1
    fi
}
"""
}

fig_assert sed

fig_export fig_log
fig_export fig_log_err
fig_export fig_assert
fig_export fig_assert_qq
fig_export fig_assert_one_q
fig_export fig_assert_one_qq

. ${1}

