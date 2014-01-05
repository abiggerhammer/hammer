# -*- coding:utf-8; -*-

# Copyright (c) 2009-10 The SCons Foundation
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#  This C# Tool taken from http://www.scons.org/wiki/CsharpBuilder and amended
#  by the patch from Issue 1912 at http://scons.tigris.org/issues/show_bug.cgi?id=1912

#  Amended and extended by Russel Winder <russel.winder@concertant.com>

#  On the SCons wiki page there are two distinct tools, one for the Microsoft C# system and one for Mono.
#  This is an attempt to meld to two based initially on the Microsoft C# tool with amendmnets from the Mono
#  tool.

import os.path
import SCons.Builder
import SCons.Node.FS
import SCons.Util
from SCons.Node.Python import Value

# needed for adding methods to environment
from SCons.Script.SConscript import SConsEnvironment

# parses env['VERSION'] for major, minor, build, and revision
def parseVersion(env):
    """parses env['VERSION'] for major, minor, build, and revision"""
    if type(env['VERSION']) is tuple or type(env['VERSION']) is list:
        major, minor, build, revision = env['VERSION']
    elif type(env['VERSION']) is str:
        major, minor, build, revision = env['VERSION'].split('.')
        major = int(major)
        minor = int(minor)
        build = int(build)
        revision = int(revision)
    return (major, minor, build, revision)

def getVersionAsmDirective(major, minor, build, revision):
    return '[assembly: AssemblyVersion("%d.%d.%d.%d")]' % (major, minor, build, revision)

def generateVersionId(env, target, source):
    out = open(target[0].path, 'w')
    out.write('using System;using System.Reflection;using System.Runtime.CompilerServices;using System.Runtime.InteropServices;\n')
    out.write(source[0].get_contents())
    out.close()

# used so that we can capture the return value of an executed command
def subprocess(cmdline):
    """used so that we can capture the return value of an executed command"""
    import subprocess
    startupinfo = subprocess.STARTUPINFO()
    startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    proc = subprocess.Popen(cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, startupinfo=startupinfo, shell=False)
    data, err = proc.communicate()
    return proc.wait(), data, err

def generatePublisherPolicyConfig(env, target, source):
    """this method assumes that source list corresponds to [0]=version, [1]=assembly base name, [2]=assembly file node"""
    # call strong name tool against compiled assembly and parse output for public token
    outputFolder = os.path.split(target[0].tpath)[0]
    pubpolicy = os.path.join(outputFolder, source[2].name)
    rv, data, err = subprocess('sn -T ' + pubpolicy)
    import re
    tok_re = re.compile(r"([a-z0-9]{16})[\r\n ]{0,3}$")
    match = tok_re.search(data)
    tok = match.group(1)

    # calculate version range to redirect from
    version = source[0].value
    oldVersionStartRange = '%s.%s.0.0' % (version[0], version[1])
    newVersion = '%s.%s.%s.%s' % (version[0], version[1], version[2], version[3])
    build = int(version[2])
    rev = int(version[3])

    # on build 0 and rev 0 or 1, no range is needed. otherwise calculate range
    if (build == 0 and (rev == 0 or rev == 1)):
        oldVersionRange = oldVersionStartRange
    else:
        if rev - 1 < 0:
            endRevisionRange = '99'
            endBuildRange = str(build-1)
        else:
            endRevisionRange = str(rev - 1)
            endBuildRange = str(build)
            oldVersionEndRange = '%s.%s.%s.%s' % (version[0], version[1], endBuildRange, endRevisionRange)
            oldVersionRange = '%s-%s' % (oldVersionStartRange, oldVersionEndRange)

    # write .net config xml out to file
    out = open(target[0].path, 'w')
    out.write('''\
 <configuration><runtime><assemblyBinding xmlns="urn:schemas-microsoft-com:asm.v1">
   <dependentAssembly>
     <assemblyIdentity name="%s" publicKeyToken="%s"/>
     <bindingRedirect oldVersion="%s" newVersion="%s"/>
   </dependentAssembly>
 </assemblyBinding></runtime></configuration>
 ''' % (source[1].value, tok, oldVersionRange, newVersion))
    out.close()

def getKeyFile(node, sources):
    """search for key file"""
    for file in node.children():
        if file.name.endswith('.snk'):
            sources.append(file)
            return

    # if not found look in included netmodules (first found is used)
    for file in node.children():
        if file.name.endswith('.netmodule'):
            for file2 in file.children():
                if file2.name.endswith('.snk'):
                    sources.append(file2)
                    return

def PublisherPolicy(env, target, **kw):
    """creates the publisher policy dll, mapping the major.minor.0.0 calls to the
    major, minor, build, and revision passed in through the dictionary VERSION key"""
    sources = []
    # get version and generate .config file
    version = parseVersion(kw)
    asm = os.path.splitext(target[0].name)[0]
    configName = 'policy.%d.%d.%s.%s' % (version[0], version[1], asm, 'config')
    targ = 'policy.%d.%d.%s' % (version[0], version[1], target[0].name)
    config = env.Command(configName, [Value(version), Value(asm), target[0]], generatePublisherPolicyConfig)
    sources.append(config[0])

    # find .snk key
    getKeyFile(target[0], sources)

    return env.CLIAsmLink(targ, sources, **kw)

def CLIRefs(env, refs, paths = [], **kw):
    listRefs = []
    normpaths = [env.Dir(p).abspath for p in paths]
    normpaths += env['CLIREFPATHS']

    for ref in refs:
        if not ref.endswith(env['SHLIBSUFFIX']):
            ref += env['SHLIBSUFFIX']
        if not ref.startswith(env['SHLIBPREFIX']):
            ref = env['SHLIBPREFIX'] + ref
        pathref = detectRef(ref, normpaths, env)
        if pathref:
            listRefs.append(pathref)

    return listRefs

def CLIMods(env, refs, paths = [], **kw):
    listMods = []
    normpaths = [env.Dir(p).abspath for p in paths]
    normpaths += env['CLIMODPATHS']

    for ref in refs:
        if not ref.endswith(env['CLIMODSUFFIX']):
            ref += env['CLIMODSUFFIX']
        pathref = detectRef(ref, normpaths, env)
        if pathref:
            listMods.append(pathref)

    return listMods

def detectRef(ref, paths, env):
    """look for existance of file (ref) at one of the paths"""
    for path in paths:
        if path.endswith(ref):
            return path
        pathref = os.path.join(path, ref)
        if os.path.isfile(pathref):
            return pathref

    return ''

def AddToRefPaths(env, files, **kw):
    # the file name is included in path reference because otherwise checks for that output file
    # by CLIRefs/CLIMods would fail until after it has been built.  Since SCons makes a pass
    # before building anything, that file won't be there.  Only after the second pass will it be built
    ref = env.FindIxes(files, 'SHLIBPREFIX', 'SHLIBSUFFIX').abspath
    env['CLIREFPATHS'] = [ref] + env['CLIREFPATHS']
    return 0

def AddToModPaths(env, files, **kw):
    mod = env.FindIxes(files, 'CLIMODPREFIX', 'CLIMODSUFFIX').abspath
    env['CLIMODPATHS'] = [mod] + env['CLIMODPATHS']
    return 0

def cscFlags(target, source, env, for_signature):
    listCmd = []
    if (env.has_key('WINEXE')):
        if (env['WINEXE'] == 1):
            listCmd.append('-t:winexe')
    return listCmd

def cscSources(target, source, env, for_signature):
    listCmd = []

    for s in source:
        if (str(s).endswith('.cs')):  # do this first since most will be source files
            listCmd.append(s)
        elif (str(s).endswith('.resources')):
            listCmd.append('-resource:%s' % s.get_string(for_signature))
        elif (str(s).endswith('.snk')):
            listCmd.append('-keyfile:%s' % s.get_string(for_signature))
        else:
            # just treat this as a generic unidentified source file
            listCmd.append(s)

    return listCmd

def cscSourcesNoResources(target, source, env, for_signature):
    listCmd = []

    for s in source:
        if (str(s).endswith('.cs')):  # do this first since most will be source files
            listCmd.append(s)
        elif (str(s).endswith('.resources')): # resources cannot be embedded in netmodules
            pass
        elif (str(s).endswith('.snk')):
            listCmd.append('-keyfile:%s' % s.get_string(for_signature))
        else:
            # just treat this as a generic unidentified source file
            listCmd.append(s)

    return listCmd

def cscRefs(target, source, env, for_signature):
    listCmd = []

    if (env.has_key('ASSEMBLYREFS')):
        refs = SCons.Util.flatten(env['ASSEMBLYREFS'])
        for ref in refs:
            if SCons.Util.is_String(ref):
                listCmd.append('-reference:%s' % ref)
            else:
                listCmd.append('-reference:%s' % ref.abspath)

    return listCmd

def cscMods(target, source, env, for_signature):
    listCmd = []

    if (env.has_key('NETMODULES')):
        mods = SCons.Util.flatten(env['NETMODULES'])
        for mod in mods:
            listCmd.append('-addmodule:%s' % mod)

    return listCmd

# TODO: this currently does not allow sources to be embedded (-embed flag)
def alLinkSources(target, source, env, for_signature):
    listCmd = []

    for s in source:
        if (str(s).endswith('.snk')):
            listCmd.append('-keyfile:%s' % s.get_string(for_signature))
        else:
            # just treat this as a generic unidentified source file
            listCmd.append('-link:%s' % s.get_string(for_signature))

    if env.has_key('VERSION'):
        version = parseVersion(env)
        listCmd.append('-version:%d.%d.%d.%d' % version)

    return listCmd

def cliLinkSources(target, source, env, for_signature):
    listCmd = []

    # append source item. if it is a netmodule and has child resources, also append those
    for s in source:
        # all source items should go into listCmd
        listCmd.append('%s' % s.get_string(for_signature))

        if (str(s).endswith('.netmodule')):
            for child in s.children():
                if child.name.endswith('.resources'):
                    listCmd.append('/assemblyresource:%s' % child.get_string(for_signature))

    return listCmd

def add_version(target, source, env):
    if env.has_key('VERSION'):
        if SCons.Util.is_String(target[0]):
            versionfile = target[0] + '_VersionInfo.cs'
        else:
            versionfile = target[0].name + '_VersionInfo.cs'
        source.append(env.Command(versionfile, [Value(getVersionAsmDirective(*parseVersion(env)))], generateVersionId))
    return (target, source)

# this check is needed because .NET assemblies like to have '.' in the name.
# scons interprets that as an extension and doesn't append the suffix as a result
def lib_emitter(target, source, env):
    newtargets = []
    for tnode in target:
        t = tnode.name
        if not t.endswith(env['SHLIBSUFFIX']):
            t += env['SHLIBSUFFIX']
        newtargets.append(t)

    return (newtargets, source)

def add_depends(target, source, env):
    """Add dependency information before the build order is established"""

    if (env.has_key('NETMODULES')):
        mods = SCons.Util.flatten(env['NETMODULES'])
        for mod in mods:
            # add as dependency
            for t in target:
                env.Depends(t, mod)

    if (env.has_key('ASSEMBLYREFS')):
        refs = SCons.Util.flatten(env['ASSEMBLYREFS'])
        for ref in refs:
            # add as dependency
            for t in target:
                env.Depends(t, ref)

    return (target, source)

csc_action = SCons.Action.Action('$CSCCOM', '$CSCCOMSTR')

MsCliBuilder = SCons.Builder.Builder(action = '$CSCCOM',
                                     source_factory = SCons.Node.FS.default_fs.Entry,
                                     emitter = add_version,
                                     suffix = '.exe')

csclib_action = SCons.Action.Action('$CSCLIBCOM', '$CSCLIBCOMSTR')

MsCliLibBuilder = SCons.Builder.Builder(action = '$CSCLIBCOM',
                                        source_factory = SCons.Node.FS.default_fs.Entry,
                                        emitter = [lib_emitter, add_version, add_depends],
                                        suffix = '$SHLIBSUFFIX')

cscmod_action = SCons.Action.Action('$CSCMODCOM', '$CSCMODCOMSTR')

MsCliModBuilder = SCons.Builder.Builder(action = '$CSCMODCOM',
                                        source_factory = SCons.Node.FS.default_fs.Entry,
                                        emitter = [add_version, add_depends],
                                        suffix = '$CLIMODSUFFIX')

def module_deps(target, source, env):
    for s in source:
        dir = s.dir.srcdir
        if (dir is not None and dir is not type(None)):
            for t in target:
                env.Depends(t,s)
    return (target, source)

clilink_action = SCons.Action.Action('$CLILINKCOM', '$CLILINKCOMSTR')

MsCliLinkBuilder = SCons.Builder.Builder(action = '$CLILINKCOM',
                                         source_factory = SCons.Node.FS.default_fs.Entry,
                                         emitter = [lib_emitter, add_version, module_deps], # don't know the best way yet to get module dependencies added
                                         suffix = '.dll') #'$SHLIBSUFFIX')

# TODO : This probably needs some more work... it hasn't been used since
# finding the abilities of the VS 2005 C++ linker for .NET.
MsCliAsmLinkBuilder = SCons.Builder.Builder(action = '$CLIASMLINKCOM',
                                            source_factory = SCons.Node.FS.default_fs.Entry,
                                            suffix = '.dll')

typelib_prefix = 'Interop.'

def typelib_emitter(target, source, env):
    newtargets = []
    for tnode in target:
        t = tnode.name
        if not t.startswith(typelib_prefix):
            t = typelib_prefix + t
        newtargets.append(t)

    return (newtargets, source)

def tlbimpFlags(target, source, env, for_signature):
    listCmd = []

    basename = os.path.splitext(target[0].name)[0]
    # strip off typelib_prefix (such as 'Interop.') so it isn't in the namespace
    if basename.startswith(typelib_prefix):
        basename = basename[len(typelib_prefix):]
    listCmd.append('-namespace:%s' % basename)

    listCmd.append('-out:%s' % target[0].tpath)

    for s in source:
        if (str(s).endswith('.snk')):
            listCmd.append('-keyfile:%s' % s.get_string(for_signature))

    return listCmd

typelibimp_action = SCons.Action.Action('$TYPELIBIMPCOM', '$TYPELIBIMPCOMSTR')

MsCliTypeLibBuilder = SCons.Builder.Builder(action = '$TYPELIBIMPCOM',
                                            source_factory = SCons.Node.FS.default_fs.Entry,
                                            emitter = [typelib_emitter, add_depends],
                                            suffix = '.dll')

res_action = SCons.Action.Action('$CLIRCCOM', '$CLIRCCOMSTR')

def res_emitter(target, source, env):
    # prepend NAMESPACE if provided
    if (env.has_key('NAMESPACE')):
        newtargets = []
        for t in target:
            tname = t.name

            # this is a cheesy way to get rid of '.aspx' in .resx file names
            idx = tname.find('.aspx.')
            if idx >= 0:
                tname = tname[:idx] + tname[idx+5:]

            newtargets.append('%s.%s' % (env['NAMESPACE'], tname))
        return (newtargets, source)
    else:
        return (targets, source)

MsCliResBuilder = SCons.Builder.Builder(action=res_action,
                                        emitter=res_emitter,
                                        src_suffix='.resx',
                                        suffix='.resources',
                                        src_builder=[],
                                        source_scanner=SCons.Tool.SourceFileScanner)

SCons.Tool.SourceFileScanner.add_scanner('.resx', SCons.Defaults.CScan)

def generate(env):
    envpaths = env['ENV']['PATH']
    env['CLIREFPATHS']  = envpaths.split(os.pathsep)
    env['CLIMODPATHS']  = []
    env['ASSEMBLYREFS'] = []
    env['NETMODULES']   = []

    env['BUILDERS']['CLIProgram'] = MsCliBuilder
    env['BUILDERS']['CLIAssembly'] = MsCliLibBuilder
    env['BUILDERS']['CLILibrary'] = MsCliLibBuilder
    env['BUILDERS']['CLIModule']  = MsCliModBuilder
    env['BUILDERS']['CLILink']    = MsCliLinkBuilder
    env['BUILDERS']['CLIAsmLink'] = MsCliAsmLinkBuilder
    env['BUILDERS']['CLIRes'] = MsCliResBuilder
    env['BUILDERS']['CLITypeLib'] = MsCliTypeLibBuilder

    env['CSC']          = env.Detect('gmcs') or 'csc'
    env['_CSCLIBS']     = "${_stripixes('-r:', CILLIBS, '', '-r', '', __env__)}"
    env['_CSCLIBPATH']  = "${_stripixes('-lib:', CILLIBPATH, '', '-r', '', __env__)}"
    env['CSCFLAGS']     = SCons.Util.CLVar('-nologo -noconfig')
    env['_CSCFLAGS']    = cscFlags
    env['_CSC_SOURCES'] = cscSources
    env['_CSC_SOURCES_NO_RESOURCES'] = cscSourcesNoResources
    env['_CSC_REFS']    = cscRefs
    env['_CSC_MODS']    = cscMods
    env['CSCCOM']       = '$CSC $CSCFLAGS $_CSCFLAGS -out:${TARGET.abspath} $_CSC_REFS $_CSC_MODS $_CSC_SOURCES'
    env['CSCLIBCOM']    = '$CSC -t:library $CSCFLAGS $_CSCFLAGS $_CSCLIBPATH $_CSCLIBS -out:${TARGET.abspath} $_CSC_REFS $_CSC_MODS $_CSC_SOURCES'
    env['CSCMODCOM']    = '$CSC -t:module $CSCFLAGS $_CSCFLAGS -out:${TARGET.abspath} $_CSC_REFS $_CSC_MODS $_CSC_SOURCES_NO_RESOURCES'
    env['CLIMODPREFIX'] = ''
    env['CLIMODSUFFIX'] = '.netmodule'
    env['CSSUFFIX']     = '.cs'

    # this lets us link .netmodules together into a single assembly
    env['CLILINK']      = 'link'
    env['CLILINKFLAGS'] = SCons.Util.CLVar('-nologo -ltcg -dll -noentry')
    env['_CLILINK_SOURCES'] = cliLinkSources
    env['CLILINKCOM']   = '$CLILINK $CLILINKFLAGS -out:${TARGET.abspath} $_CLILINK_SOURCES' # $SOURCES'

    env['CLIASMLINK']   = 'al'
    env['CLIASMLINKFLAGS'] = SCons.Util.CLVar('')
    env['_ASMLINK_SOURCES'] = alLinkSources
    env['CLIASMLINKCOM'] = '$CLIASMLINK $CLIASMLINKFLAGS -out:${TARGET.abspath} $_ASMLINK_SOURCES'

    env['CLIRC']        = 'resgen'
    env['CLIRCFLAGS']   = ''
    env['CLIRCCOM']     = '$CLIRC $CLIRCFLAGS $SOURCES $TARGETS'

    env['TYPELIBIMP']       = 'tlbimp'
    env['TYPELIBIMPFLAGS'] = SCons.Util.CLVar('-sysarray')
    env['_TYPELIBIMPFLAGS'] = tlbimpFlags
    env['TYPELIBIMPCOM']    = '$TYPELIBIMP $SOURCES $TYPELIBIMPFLAGS $_TYPELIBIMPFLAGS'

    SConsEnvironment.CLIRefs = CLIRefs
    SConsEnvironment.CLIMods = CLIMods
    SConsEnvironment.AddToRefPaths = AddToRefPaths
    SConsEnvironment.AddToModPaths = AddToModPaths
    SConsEnvironment.PublisherPolicy = PublisherPolicy

def exists(env):
    return env.Detect('csc') or env.Detect('gmcs')
