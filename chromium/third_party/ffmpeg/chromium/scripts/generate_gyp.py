#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates a GYP include file for building FFmpeg from source.

The way this works is a bit silly but it's easier than reverse engineering
FFmpeg's configure scripts and Makefiles. It scans through build directories for
object files then does a reverse lookup against the FFmpeg source tree to find
the corresponding C or assembly file.

Running build_ffmpeg.sh for ia32, arm, and arm-neon platforms is required
prior to running this script. The arm and arm-neon platforms assume a
Chromium OS build environment.

Step 1: Have a Chromium OS checkout (refer to http://dev.chromium.org)
  mkdir chromeos
  repo init ...
  repo sync

Step 2: Check out deps/third_party/ffmpeg inside Chromium OS (or cp -fpr it over
from an existing checkout; symlinks and mount --bind no longer appear to enable
access from within chroot)
  cd path/to/chromeos
  mkdir deps
  cd deps
  git clone http://git.chromium.org/chromium/third_party/ffmpeg.git

Step 3: Build for ia32 platform outside chroot (will need yasm in path)
  cd path/to/chromeos/deps/ffmpeg
  ./chromium/scripts/build_ffmpeg.sh linux ia32 path/to/chromeos/deps/ffmpeg

Step 4: Build and enter Chromium OS chroot:
  cd path/to/chromeos/src/scripts
  cros_sdk --enter

Step 5: Setup build environment for ARM:
  ./setup_board --board arm-generic

Step 6: Build for arm/arm-neon platforms inside chroot
  ./chromium/scripts/build_ffmpeg.sh linux arm path/to/chromeos/deps/ffmpeg
  ./chromium/scripts/build_ffmpeg.sh linux arm-neon path/to/chromeos/deps/ffmpeg

Step 7: Build for Windows platform; you will need a MinGW shell started from
inside a Visual Studio Command Prompt to run build_ffmpeg.sh:
  ./chromium/scripts/build_ffmpeg.sh win ia32 $(pwd)

Step 8: Exit chroot and generate gyp file
  exit
  cd path/to/chromeos/deps/ffmpeg
  ./chromium/scripts/generate_gyp.py

Phew!

While this seems insane, reverse engineering and maintaining a gyp file by hand
is significantly more painful.
"""

__author__ = 'scherkus@chromium.org (Andrew Scherkus)'

import datetime
import fnmatch
import itertools
import optparse
import os
import string

COPYRIGHT = """# Copyright %d The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# NOTE: this file is autogenerated by ffmpeg/chromium/scripts/generate_gyp.py

""" % (datetime.datetime.now().year)

GYP_HEADER = """
{
  'variables': {
"""

GYP_FOOTER = """  },
}
"""


GYP_CONDITIONAL_BEGIN = """    'conditions': [
"""
GYP_CONDITIONAL_END = """    ],  # conditions
"""
GYP_CONDITIONAL_STANZA_BEGIN = """      ['%s', {
"""
GYP_CONDITIONAL_STANZA_ITEM = """          '%s',
"""
GYP_CONDITIONAL_STANZA_OUTPUT_ITEM = """          '<(shared_generated_dir)/%s',
"""
GYP_CONDITIONAL_STANZA_END = """      }],  # %s
"""

GYP_CONDITIONAL_C_SOURCE_STANZA_BEGIN = """        'c_sources': [
"""
GYP_CONDITIONAL_ASM_SOURCE_STANZA_BEGIN = """        'asm_sources': [
"""
GYP_CONDITIONAL_ITEM_STANZA_END = """        ],
"""

GN_HEADER = """import("//build/config/arm.gni")
import("ffmpeg_options.gni")

# Declare empty versions of each variable for easier +=ing later.
ffmpeg_c_sources = []
ffmpeg_gas_sources = []
ffmpeg_yasm_sources = []

"""
GN_CONDITION_BEGIN = """if (%s) {
"""
GN_CONDITION_END = """}

"""
GN_C_SOURCES_BEGIN = """ffmpeg_c_sources += [
"""
GN_GAS_SOURCES_BEGIN = """ffmpeg_gas_sources += [
"""
GN_YASM_SOURCES_BEGIN = """ffmpeg_yasm_sources += [
"""
GN_SOURCE_ITEM = """  "%s",
"""
GN_SOURCE_END = """]
"""

# Controls GYP conditional stanza generation.
SUPPORTED_ARCHITECTURES = ['ia32', 'arm', 'arm-neon']
SUPPORTED_TARGETS = ['Chromium', 'Chrome', 'ChromiumOS', 'ChromeOS']
# Mac doesn't have any platform specific files, so just use linux and win.
SUPPORTED_PLATFORMS = ['linux', 'win']


def NormalizeFilename(name):
  """ Removes leading path separators in an attempt to normalize paths."""
  return string.lstrip(name, os.sep)


def CleanObjectFiles(object_files):
  """Removes unneeded object files due to linker errors, binary size, etc...

  Args:
    object_files: List of object files that needs cleaning.
  """
  blacklist = [
      'libavcodec/inverse.o',  # Includes libavutil/inverse.c
      'libavcodec/file_open.o', # Includes libavutil/file_open.c
      'libavcodec/log2_tab.o',  # Includes libavutil/log2_tab.c
      'libavformat/golomb_tab.o',  # Includes libavcodec/golomb.c
      'libavformat/log2_tab.o',  # Includes libavutil/log2_tab.c
      'libavformat/file_open.o', # Includes libavutil/file_open.c

      # The following files are removed to trim down on binary size.
      # TODO(ihf): Warning, it is *easy* right now to remove more files
      # than is healthy and end up with a library that the linker does
      # not complain about but that can't be loaded. Add some verification!
      'libavcodec/audioconvert.o',
      'libavcodec/resample.o',
      'libavcodec/resample2.o',
      'libavcodec/x86/dnxhd_mmx.o',
      'libavformat/sdp.o',
      'libavutil/adler32.o',
      'libavutil/audio_fifo.o',
      'libavutil/aes.o',
      'libavutil/blowfish.o',
      'libavutil/des.o',
      'libavutil/file.o',
      'libavutil/hash.o',
      'libavutil/hmac.o',
      'libavutil/lls.o',
      'libavutil/murmur3.o',
      'libavutil/rc4.o',
      'libavutil/ripemd.o',
      'libavutil/sha512.o',
      'libavutil/tree.o',
      'libavutil/xtea.o',
      'libavutil/xga_font_data.o',
  ]
  for name in blacklist:
    name = name.replace('/', os.sep)
    if name in object_files:
      object_files.remove(name)
  return object_files

def IsAssemblyFile(f):
  _, ext = os.path.splitext(f)
  return ext in ['.S', '.asm']

def IsGasFile(f):
  _, ext = os.path.splitext(f)
  return ext in ['.S']

def IsYasmFile(f):
  _, ext = os.path.splitext(f)
  return ext in ['.asm']

def IsCFile(f):
  _, ext = os.path.splitext(f)
  return ext in ['.c']

def IsSourceFile(f):
  return IsAssemblyFile(f) or IsCFile(f)

def GetSourceFiles(source_dir):
  """Returns a list of source files for the given source directory.

  Args:
    source_dir: Path to build a source mapping for.

  Returns:
    A python list of source file paths.
  """

  def IsSourceDir(d):
    return d not in ['.git', '.svn']

  source_files = []
  for root, dirs, files in os.walk(source_dir):
    dirs = filter(IsSourceDir, dirs)
    files = filter(IsSourceFile, files)

    # Strip leading source_dir from root.
    root = root[len(source_dir):]
    source_files.extend([NormalizeFilename(os.path.join(root, name)) for name in
                         files])
  return source_files


def GetObjectFiles(build_dir):
  """Returns a list of object files for the given build directory.

  Args:
    build_dir: Path to build an object file list for.

  Returns:
    A python list of object files paths.
  """
  object_files = []
  for root, dirs, files in os.walk(build_dir):
    # Strip leading build_dir from root.
    root = root[len(build_dir):]

    for name in files:
      _, ext = os.path.splitext(name)
      if ext == '.o':
        name = NormalizeFilename(os.path.join(root, name))
        object_files.append(name)
  CleanObjectFiles(object_files)
  return object_files


def GetObjectToSourceMapping(source_files):
  """Returns a map of object file paths to source file paths.

  Args:
    source_files: List of source file paths.

  Returns:
    Map with object file paths as keys and source file paths as values.
  """
  object_to_sources = {}
  for name in source_files:
    basename, ext = os.path.splitext(name)
    key = basename + '.o'
    object_to_sources[key] = name
  return object_to_sources


def GetSourceFileSet(object_to_sources, object_files):
  """Determines set of source files given object files.

  Args:
    object_to_sources: A dictionary of object to source file paths.
    object_files: A list of object file paths.

  Returns:
    A python set of source files requried to build said objects.
  """
  source_set = set()
  for name in object_files:
    # Intentially raise a KeyError if lookup fails since something is messed
    # up with our source and object lists.
    source_set.add(object_to_sources[name])
  return source_set


class SourceSet(object):
  """A SourceSet represents a set of source files that are built on the given
  set of architectures and targets.
  """

  def __init__(self, sources, architectures, targets, platforms):
    """Creates a SourceSet.

    Args:
      sources: a python set of source files
      architectures: a python set of architectures (i.e., arm, x64)
      targets: a python set of targets (i.e., Chromium, Chrome)
      platforms: a python set of platforms (i.e., win, linux)
    """
    self.sources = sources
    self.architectures = architectures
    self.targets = targets
    self.platforms = platforms

  def __repr__(self):
    return '{%s, %s, %s, %s}' % (self.sources, self.architectures, self.targets,
                                 self.platforms)

  def __eq__(self, other):
    return (self.sources == other.sources and
            self.architectures == other.architectures and
            self.targets == other.targets and
            self.platforms == other.platforms)

  def Intersect(self, other):
    """Return a new SourceSet containing the set of source files common to both
    this and the other SourceSet.

    The resulting SourceSet represents the union of the architectures and
    targets of this and the other SourceSet.
    """
    return SourceSet(self.sources & other.sources,
                     self.architectures | other.architectures,
                     self.targets | other.targets,
                     self.platforms | other.platforms)

  def Difference(self, other):
    """Return a new SourceSet containing the set of source files not present in
    the other SourceSet.

    The resulting SourceSet represents the intersection of the architectures and
    targets of this and the other SourceSet.
    """
    return SourceSet(self.sources - other.sources,
                     self.architectures & other.architectures,
                     self.targets & other.targets,
                     self.platforms & other.platforms)

  def IsEmpty(self):
    """An empty SourceSet is defined as containing no source files or no
    architecture/target (i.e., a set of files that aren't built on anywhere).
    """
    return (len(self.sources) == 0 or len(self.architectures) == 0 or
            len(self.targets) == 0 or len(self.platforms) == 0)

  def GenerateGypStanza(self):
    """Generates a gyp conditional stanza representing this source set.

    TODO(scherkus): Having all this special case condition optimizing logic in
    here feels a bit dirty, but hey it works. Perhaps refactor if it starts
    getting out of hand.

    Returns:
      A string of gyp code.
    """

    # Only build a non-trivial conditional if it's a subset of all supported
    # architectures.
    arch_conditions = []
    if self.architectures == set(SUPPORTED_ARCHITECTURES + ['x64']):
      arch_conditions.append('1')
    else:
      for arch in self.architectures:
        if arch == 'arm-neon':
          arch_conditions.append('(target_arch == "arm" and arm_neon == 1)')
        else:
          arch_conditions.append('target_arch == "%s"' % arch)

    # Only build a non-trivial conditional if it's a subset of all supported
    # targets.
    branding_conditions = []
    if self.targets == set(SUPPORTED_TARGETS):
      branding_conditions.append('1')
    else:
      for branding in self.targets:
        branding_conditions.append('ffmpeg_branding == "%s"' % branding)

    platform_conditions = []
    if (self.platforms == set(SUPPORTED_PLATFORMS) or
        self.platforms == set(['linux'])):
      platform_conditions.append('1')
    else:
      for platform in self.platforms:
        platform_conditions.append('OS == "%s"' % platform)

    conditions = '(%s) and (%s) and (%s)' % (' or '.join(arch_conditions),
                                             ' or '.join(branding_conditions),
                                             ' or '.join(platform_conditions))

    stanza = []
    stanza += GYP_CONDITIONAL_STANZA_BEGIN % (conditions)

    self.sources = sorted(n.replace('\\', '/') for n in self.sources)

    # Write out all C sources.
    c_sources = filter(IsCFile, self.sources)
    if c_sources:
      stanza += GYP_CONDITIONAL_C_SOURCE_STANZA_BEGIN
      for name in c_sources:
        stanza += GYP_CONDITIONAL_STANZA_ITEM % (name)
      stanza += GYP_CONDITIONAL_ITEM_STANZA_END

    # Write out all assembly sources.
    asm_sources = filter(IsAssemblyFile, self.sources)
    if asm_sources:
      stanza += GYP_CONDITIONAL_ASM_SOURCE_STANZA_BEGIN
      for name in asm_sources:
        stanza += GYP_CONDITIONAL_STANZA_ITEM % (name)
      stanza += GYP_CONDITIONAL_ITEM_STANZA_END

    stanza += GYP_CONDITIONAL_STANZA_END % (conditions)
    return ''.join(stanza)

  def WriteGnStanza(self, fd):
    """Generates a gyp conditional stanza representing this source set.

    TODO(scherkus): Having all this special case condition optimizing logic in
    here feels a bit dirty, but hey it works. Perhaps refactor if it starts
    getting out of hand.

    Args:
      fd: File object to write the stanza into.
    """

    # Only build a non-trivial conditional if it's a subset of all supported
    # architectures. targets. Arch conditions look like:
    #   (cpu_arch == "arm" || (cpu_arch == "arm" && arm_use_neon))
    arch_conditions = []
    if self.architectures != set(SUPPORTED_ARCHITECTURES + ['x64']):
      for arch in self.architectures:
        if arch == 'arm-neon':
          arch_conditions.append('(cpu_arch == "arm" && arm_use_neon)')
        else:
          arch_conditions.append('cpu_arch == "%s"' % arch)

    # Only build a non-trivial conditional if it's a subset of all supported
    # targets. Branding conditions look like:
    #   (ffmpeg_branding == "Chrome" || ffmpeg_branding == "ChromeOS")
    branding_conditions = []
    if self.targets != set(SUPPORTED_TARGETS):
      for branding in self.targets:
        branding_conditions.append('ffmpeg_branding == "%s"' % branding)

    # Platform conditions look like:
    #   (os == "mac" || os == "linux")
    platform_conditions = []
    if (self.platforms != set(SUPPORTED_PLATFORMS) and
        self.platforms != set(['linux'])):
      for platform in self.platforms:
        platform_conditions.append('os == "%s"' % platform)

    # Remove 0-lengthed lists.
    conditions = filter(None, [' || '.join(arch_conditions),
                               ' || '.join(branding_conditions),
                               ' || '.join(platform_conditions)])

    # If there is more that one clause, wrap various conditions in parens
    # before joining.
    if len(conditions) > 1:
       conditions = [ '(%s)' % x for x in conditions ]

    # Output a conditional wrapper around stanzas if necessary.
    if conditions:
      fd.write(GN_CONDITION_BEGIN % ' && '.join(conditions))
      def indent_write(s):
        fd.write('  %s' % s)
    else:
      def indent_write(s):
        fd.write(s)

    sources = sorted(n.replace('\\', '/') for n in self.sources)

    # Write out all C sources.
    c_sources = filter(IsCFile, sources)
    if c_sources:
      indent_write(GN_C_SOURCES_BEGIN)
      for name in c_sources:
        indent_write(GN_SOURCE_ITEM % (name))
      indent_write(GN_SOURCE_END)

    # Write out all assembly sources.
    gas_sources = filter(IsGasFile, sources)
    if gas_sources:
      indent_write(GN_GAS_SOURCES_BEGIN)
      for name in gas_sources:
        indent_write(GN_SOURCE_ITEM % (name))
      indent_write(GN_SOURCE_END)

    # Write out all assembly sources.
    yasm_sources = filter(IsYasmFile, sources)
    if yasm_sources:
      indent_write(GN_YASM_SOURCES_BEGIN)
      for name in yasm_sources:
        indent_write(GN_SOURCE_ITEM % (name))
      indent_write(GN_SOURCE_END)

    # Close the conditional if necessary.
    if conditions:
      fd.write(GN_CONDITION_END)
    else:
      fd.write('\n')  # Makeup the spacing for the remove conditional.


def CreatePairwiseDisjointSets(sets):
  """ Given a list of SourceSet objects, returns the pairwise disjoint sets.

  NOTE: This isn't the most efficient algorithm, but given how infrequent we
  need to run this and how small the input size is we'll leave it as is.
  """

  disjoint_sets = list(sets)

  new_sets = True
  while new_sets:
    new_sets = False
    for pair in itertools.combinations(disjoint_sets, 2):
      intersection = pair[0].Intersect(pair[1])

      # Both pairs are already disjoint, nothing to do.
      if intersection.IsEmpty():
        continue

      # Add the resulting intersection set.
      new_sets = True
      disjoint_sets.append(intersection)

      # Calculate the resulting differences for this pair of sets.
      #
      # If the differences are an empty set, remove them from the list of sets,
      # otherwise update the set itself.
      for p in pair:
        i = disjoint_sets.index(p)
        difference = p.Difference(intersection)
        if difference.IsEmpty():
          del disjoint_sets[i]
        else:
          disjoint_sets[i] = difference

      # Restart the calculation since the list of disjoint sets has changed.
      break

  return disjoint_sets


def ParseOptions():
  """Parses the options and terminates program if they are not sane.

  Returns:
    The pair (optparse.OptionValues, [string]), that is the output of
    a successful call to parser.parse_args().
  """
  parser = optparse.OptionParser(
      usage='usage: %prog [options] DIR')

  parser.add_option('-s',
                    '--source_dir',
                    dest='source_dir',
                    default='.',
                    metavar='DIR',
                    help='FFmpeg source directory.')

  parser.add_option('-b',
                    '--build_dir',
                    dest='build_dir',
                    default='.',
                    metavar='DIR',
                    help='Build root containing build.x64.linux, etc...')

  parser.add_option('-g',
                    '--output_gn',
                    dest='output_gn',
                    action="store_true",
                    default=False,
                    help='Output a GN file instead of a gyp file.')

  options, args = parser.parse_args()

  if not options.source_dir:
    parser.error('No FFmpeg source directory specified')

  if not os.path.exists(options.source_dir):
    parser.error('FFmpeg source directory does not exist')

  if not options.build_dir:
    parser.error('No build root directory specified')

  return options, args


def WriteGyp(fd, build_dir, disjoint_sets):
  fd.write(COPYRIGHT)
  fd.write(GYP_HEADER)

  # Generate conditional stanza for each disjoint source set.
  fd.write(GYP_CONDITIONAL_BEGIN)
  for s in disjoint_sets:
    fd.write(s.GenerateGypStanza())
  fd.write(GYP_CONDITIONAL_END)

  fd.write(GYP_FOOTER)


def WriteGn(fd, build_dir, disjoint_sets):
  fd.write(COPYRIGHT)
  fd.write(GN_HEADER)

  # Generate conditional stanza for each disjoint source set.
  for s in reversed(disjoint_sets):
    s.WriteGnStanza(fd)


def main():
  options, args = ParseOptions()

  # Generate map of FFmpeg source files.
  source_dir = options.source_dir
  source_files = GetSourceFiles(source_dir)
  object_to_sources = GetObjectToSourceMapping(source_files)

  sets = []

  for arch in SUPPORTED_ARCHITECTURES:
    for target in SUPPORTED_TARGETS:
      for platform in SUPPORTED_PLATFORMS:
        # Assume build directory is of the form build.$arch.$platform/$target.
        name = ''.join(['build.', arch, '.', platform])
        build_dir = os.path.join(options.build_dir, name, target)
        if not os.path.exists(build_dir):
          continue
        print 'Processing build directory: %s' % name

        object_files = GetObjectFiles(build_dir)

        # Generate the set of source files to build said target.
        s = GetSourceFileSet(object_to_sources, object_files)
        sets.append(SourceSet(s, set([arch]), set([target]), set([platform])))

        # x64 and ia32 are the same file set.
        if arch == 'ia32':
          sets.append(SourceSet(s, set(['x64']), set([target]),
                      set([platform])))

  sets = CreatePairwiseDisjointSets(sets)
  # Open for writing.
  if options.output_gn:
    outfile = 'ffmpeg_generated.gni'
  else:
    outfile = 'ffmpeg_generated.gypi'
  output_name = os.path.join(options.source_dir, outfile)
  with open(output_name, 'w') as fd:
    if options.output_gn:
      WriteGn(fd, options.build_dir, sets)
    else:
      WriteGyp(fd, options.build_dir, sets)

if __name__ == '__main__':
  main()
