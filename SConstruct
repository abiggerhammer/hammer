# -*- python -*-
import os
import os.path
import sys


vars = Variables(None, ARGUMENTS)
vars.Add(PathVariable('DESTDIR', "Root directory to install in (useful for packaging scripts)", None, PathVariable.PathIsDirCreate))
vars.Add(PathVariable('prefix', "Where to install in the FHS", "/usr/local", PathVariable.PathAccept))
vars.Add(ListVariable('bindings', 'Language bindings to build', 'none', ['php']))

env = Environment(ENV = {'PATH' : os.environ['PATH']}, variables = vars, tools=['default', 'scanreplace'], toolpath=['tools'])

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


env['libpath'] = calcInstallPath("$prefix", "lib")
env['incpath'] = calcInstallPath("$prefix", "include", "hammer")
env['pkgconfigpath'] = calcInstallPath("$prefix", "lib", "pkgconfig")
env.ScanReplace('libhammer.pc.in')

env.MergeFlags("-std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-attributes")

if not env['PLATFORM'] == 'darwin':
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

env['BUILDDIR'] = 'build/$VARIANT'

dbg = env.Clone(VARIANT='debug')
dbg.Append(CCFLAGS=['-g'])

opt = env.Clone(VARIANT='opt')
opt.Append(CCFLAGS="-O3")

if GetOption("variant") == 'debug':
    env = dbg
else:
    env = opt

if GetOption("coverage"):
    env.Append(CFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               CXXFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               LDFLAGS=["-fprofile-arcs", "-ftest-coverage"],
               LIBS=['gcov'])

env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]

if os.getenv("CC") == "clang" or env['PLATFORM'] == 'darwin':
    env.Replace(CC="clang",
                CXX="clang++")

env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

#rootpath = env['ROOTPATH'] = os.path.abspath('.')
#env.Append(CPPPATH=os.path.join('#', "hammer"))

Export('env')

lib = env.SConscript(["src/SConscript"], variant_dir='build/$VARIANT/src')
Default(lib)

env.Alias("examples", env.SConscript(["examples/SConscript"], variant_dir='build/$VARIANT/examples'))

env.Alias("test", env.Command('test', 'build/$VARIANT/src/test_suite', 'env LD_LIBRARY_PATH=build/$VARIANT/src $SOURCE'))

env.Alias("install", "$libpath")
env.Alias("install", "$incpath")
env.Alias("install", "$pkgconfigpath")
