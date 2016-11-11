# -*- python -*-
import os
import os.path
import platform
import sys
from distutils.version import LooseVersion
import re
import subprocess


vars = Variables(None, ARGUMENTS)
vars.Add(PathVariable('DESTDIR', "Root directory to install in (useful for packaging scripts)", None, PathVariable.PathIsDirCreate))
vars.Add(PathVariable('prefix', "Where to install in the FHS", "/usr/local", PathVariable.PathAccept))
vars.Add(ListVariable('bindings', 'Language bindings to build', 'none', ['cpp', 'dotnet', 'perl', 'php', 'python', 'ruby']))

tools = ['default', 'scanreplace']
if 'dotnet' in ARGUMENTS.get('bindings', []):
	tools.append('csharp/mono')

envvars = {'PATH' : os.environ['PATH']}
if 'PKG_CONFIG_PATH' in os.environ:
    envvars['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']

env = Environment(ENV = envvars,
                  variables = vars,
                  tools=tools,
                  toolpath=['tools'])

if not 'bindings' in env:
    env['bindings'] = []

def calcInstallPath(*elements):
    path = os.path.abspath(os.path.join(*map(env.subst, elements)))
    if 'DESTDIR' in env:
        path = os.path.join(env['DESTDIR'], os.path.relpath(path, start="/"))
    return path

rel_prefix = not os.path.isabs(env['prefix'])
env['prefix'] = os.path.abspath(env['prefix'])
if 'DESTDIR' in env:
    env['DESTDIR'] = os.path.abspath(env['DESTDIR'])
    if rel_prefix:
        print >>sys.stderr, "--!!-- You used a relative prefix with a DESTDIR. This is probably not what you"
        print >>sys.stderr, "--!!-- you want; files will be installed in"
        print >>sys.stderr, "--!!--    %s" % (calcInstallPath("$prefix"),)

env['LLVM_CONFIG'] = "llvm-config"
env['libpath'] = calcInstallPath("$prefix", "lib")
env['incpath'] = calcInstallPath("$prefix", "include", "hammer")
env['parsersincpath'] = calcInstallPath("$prefix", "include", "hammer", "parsers")
env['backendsincpath'] = calcInstallPath("$prefix", "include", "hammer", "backends")
env['pkgconfigpath'] = calcInstallPath("$prefix", "lib", "pkgconfig")
env.ScanReplace('libhammer.pc.in')

env.MergeFlags("-std=gnu11 -Wno-unused-parameter -Wno-attributes -Wno-unused-variable -Wall -Wextra -Werror")

if env['PLATFORM'] == 'darwin':
    env.Append(SHLINKFLAGS = '-install_name ' + env["libpath"] + '/${TARGET.file}')
elif os.uname()[0] == "OpenBSD":
    pass
else:
    env.MergeFlags("-lrt")

AddOption("--variant",
          dest="variant",
          nargs=1, type="choice",
          choices=["debug", "opt"],
          default="opt",
          action="store",
          help="Build variant (debug or opt)")

AddOption("--coverage",
          dest="coverage",
          default=False,
          action="store_true",
          help="Build with coverage instrumentation")

AddOption("--in-place",
          dest="in_place",
          default=False,
          action="store_true",
          help="Build in-place, rather than in the build/<variant> tree")


dbg = env.Clone(VARIANT='debug')
dbg.MergeFlags("-g -O0")

opt = env.Clone(VARIANT='opt')
opt.Append(CCFLAGS=["-O3"])

if GetOption("variant") == 'debug':
    env = dbg
else:
    env = opt

env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]
env["LLVM_CONFIG"] = os.getenv("LLVM_CONFIG") or env["LLVM_CONFIG"]

if GetOption("coverage"):
    env.Append(CFLAGS=["--coverage"],
               CXXFLAGS=["--coverage"],
               LDFLAGS=["--coverage"])
    if env["CC"] == "gcc":
        env.Append(LIBS=['gcov'])
    else:
        env.ParseConfig('%s --cflags --ldflags --libs core executionengine mcjit analysis x86codegen x86info' % \
                        env["LLVM_CONFIG"])

if os.getenv("CC") == "clang" or env['PLATFORM'] == 'darwin':
    env.Replace(CC="clang",
                CXX="clang++")

env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

#rootpath = env['ROOTPATH'] = os.path.abspath('.')
#env.Append(CPPPATH=os.path.join('#', "hammer"))

testruns = []

targets = ["$libpath",
           "$incpath",
           "$parsersincpath",
           "$backendsincpath",
           "$pkgconfigpath"]

# Set up LLVM config stuff to export

# some llvm versions are old and will not work; some require --system-libs
# with llvm-config, and some will break if given it
llvm_config_version = subprocess.Popen('%s --version' % env["LLVM_CONFIG"], \
                                       shell=True, \
                                       stdin=subprocess.PIPE, stdout=subprocess.PIPE).communicate()
if LooseVersion(llvm_config_version[0]) < LooseVersion("3.6"):
   print "This LLVM version %s is too old" % llvm_config_version
   Exit(1)

if LooseVersion(llvm_config_version[0]) < LooseVersion("3.9") and \
   LooseVersion(llvm_config_version[0]) >= LooseVersion("3.5"):
    llvm_system_libs_flag = "--system-libs"
else:
    llvm_system_libs_flag = ""

# Only keep one copy of this
llvm_required_components = "core executionengine mcjit analysis x86codegen x86info"
# Stubbing this out so we can implement static-only mode if needed later
llvm_use_shared = True
# Can we ask for shared/static from llvm-config?
if LooseVersion(llvm_config_version[0]) < LooseVersion("3.9"):
    # Nope
    llvm_linkage_type_flag = ""
    llvm_use_computed_shared_lib_name = True
else:
    # Woo, they finally fixed the dumb
    llvm_use_computed_shared_lib_name = False
    if llvm_use_shared:
        llvm_linkage_type_flag = "--link-shared"
    else:
        llvm_linkage_type_flag = "--link-static"

if llvm_use_computed_shared_lib_name:
    # Okay, pull out the major and minor version numbers (barf barf)
    p = re.compile("^(\d+)\.(\d+).*$")
    m = p.match(llvm_config_version[0])
    if m:
        llvm_computed_shared_lib_name = "LLVM-%d.%d" % ((int)(m.group(1)), (int)(m.group(2)))
    else:
        print "Couldn't compute shared library name from LLVM version '%s', but needed to" % \
            llvm_config_version[0]
        Exit(1)
else:
    # We won't be needing it
    llvm_computed_shared_lib_name = None

# llvm-config 'helpfully' supplies -g and -O flags; educate it with this
# custom ParseConfig function arg; make it a class with a method so we can
# pass it around with scons export/import

class LLVMConfigSanitizer:
    def sanitize(self, env, cmd, unique=1):
        # cmd is output from llvm-config
        flags = cmd.split()
        # match -g or -O flags
        p = re.compile("^-[gO].*$")
        filtered_flags = [flag for flag in flags if not p.match(flag)]
        filtered_cmd = ' '.join(filtered_flags)
        # print "llvm_config_sanitize: \"%s\" => \"%s\"" % (cmd, filtered_cmd)
        env.MergeFlags(filtered_cmd, unique)
llvm_config_sanitizer = LLVMConfigSanitizer()

Export('env')
Export('testruns')
Export('targets')
# LLVM-related flags
Export('llvm_computed_shared_lib_name')
Export('llvm_config_sanitizer')
Export('llvm_config_version')
Export('llvm_linkage_type_flag')
Export('llvm_required_components')
Export('llvm_system_libs_flag')
Export('llvm_use_computed_shared_lib_name')
Export('llvm_use_shared')

if not GetOption("in_place"):
    env['BUILD_BASE'] = 'build/$VARIANT'
    lib = env.SConscript(["src/SConscript"], variant_dir='$BUILD_BASE/src')
    env.Alias("examples", env.SConscript(["examples/SConscript"], variant_dir='$BUILD_BASE/examples'))
else:
    env['BUILD_BASE'] = '.'
    lib = env.SConscript(["src/SConscript"])
    env.Alias(env.SConscript(["examples/SConscript"]))

for testrun in testruns:
    env.Alias("test", testrun)

env.Alias("install", targets)
