#!/usr/bin/env python3


import os, os.path, re
import subprocess as sp
import urllib, urllib.request
import tarfile


def bootstrap_boost(paths):
    dep = Dependency()
    dep.description = 'Boost C++ Libraries'
    dep.name, dep.version = 'boost', '1.72.0'
    url_fmt = 'https://dl.bintray.com/boostorg/release/{version}/source/boost_{version_sub}.tar.gz'
    dep.url = url_fmt.format(version=dep.version, version_sub=dep.version.replace('.', '_'))

    download_and_extract(dep, paths)

    # create the symlink used by cmake
    link_path = os.path.join(paths.deps_sources, 'boost')
    if not os.path.lexists(link_path):
        os.symlink(dep.source_path, os.path.join(paths.deps_sources, 'boost'))

    #os.chdir(dep.source_path)

    #sp.run([
    #    './bootstrap.sh',
    #    '--prefix=%s' % paths.deps_prefix,
    #    '--without-icu',
    #    '--with-python=python3',
    #    '--with-libraries=container,headers',
    #])

    #sp.run([
    #    './b2',
    #    'install',
    #])

def bootstrap_catch(paths):
    dep = Dependency()
    dep.description = 'Catch2 Unit Testing Framework'
    dep.name, dep.version = 'Catch2', '2.11.1'
    url_fmt = 'https://github.com/catchorg/Catch2/archive/v{version}.tar.gz'
    dep.url = url_fmt.format(version=dep.version)

    download_and_extract(dep, paths)

    sp.run([
        'cmake',
        '-GNinja', # TODO: make conditional
        '-S%s' % dep.source_path,
        '-B%s' % dep.build_path,
        '-DCMAKE_INSTALL_PREFIX=%s' % paths.deps_prefix,
        '-DBUILD_TESTING=OFF',
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCATCH_BUILD_EXAMPLES=OFF',
        '-DCATCH_BUILD_EXTRA_TESTS=OFF',
        '-DCATCH_BUILD_TESTING=OFF',
        '-DCATCH_ENABLE_COVERAGE=OFF',
        '-DCATCH_ENABLE_WERROR=OFF',
        '-DCATCH_INSTALL_DOCS=OFF',
        '-DCATCH_INSTALL_HELPERS=OFF',
        '-DCATCH_USE_VALGRIND=OFF',
    ])

    sp.run([
        'ninja',
        '-C%s' % dep.build_path,
        'install',
    ])


def bootstrap_eigen(paths):
    dep = Dependency()
    dep.description = 'Eigen Math Library'
    dep.name, dep.version = 'Eigen', '3.3.7'
    url_fmt = 'https://gitlab.com/libeigen/eigen/-/archive/{version}/eigen-{version}.tar.gz'
    dep.url = url_fmt.format(version=dep.version)

    download_and_extract(dep, paths)

    sp.run([
        'cmake',
        '-GNinja', # TODO: make conditional
        '-S%s' % dep.source_path,
        '-B%s' % dep.build_path,
        '-DCMAKE_INSTALL_PREFIX=%s' % paths.deps_prefix,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DBUILD_TESTING=OFF',
        '-DEIGEN_BUILD_BTL=OFF',
        '-DEIGEN_COVERAGE_TESTING=OFF',
        '-DEIGEN_DEBUG_ASSERTS=OFF',
        '-DEIGEN_DEFAULT_TO_ROW_MAJOR=OFF',
        '-DEIGEN_FAILTEST=OFF',
        '-DEIGEN_INTERNAL_DOCUMENTATION=OFF',
    ])

    sp.run([
        'ninja',
        '-C%s' % dep.build_path,
        'install',
    ])


def main(argv):
    paths = BootstrapPaths()
    paths.project, _ = os.path.split(os.path.realpath(__file__))
    paths.deps = os.path.join(paths.project, 'deps')
    paths.deps_sources = os.path.join(paths.deps, 'sources')
    paths.deps_build = os.path.join(paths.deps, 'build')
    paths.deps_prefix = os.path.join(paths.deps, 'prefix')

    os.makedirs(paths.deps_sources, exist_ok=True)
    os.makedirs(paths.deps_build, exist_ok=True)
    os.makedirs(paths.deps_prefix, exist_ok=True)

    for k, v in paths.items():
        print(k, v)

    bootstrap_boost(paths)
    bootstrap_catch(paths)
    bootstrap_eigen(paths)


def download_and_extract(dep, paths):
    from urllib.request import urlopen

    os.chdir(paths.deps_sources)

    archive_name = '%s.%s.tar.gz' % (dep.name, dep.version)
    dep.archive_path = os.path.join(paths.deps_sources, archive_name) 

    if not os.path.lexists(dep.archive_path):
        print('DOWNLOAD %r AS %r ...' % (dep.url, archive_name))
        with urlopen(url=dep.url) as response:
            with open(dep.archive_path, 'wb+') as archive_file:
                archive_file.write(response.read())

    print('EXTRACT %r ...' % archive_name)
    with tarfile.open(name=dep.archive_path, mode='r:*') as tar:
        common_path = os.path.commonpath(tar.getnames())
        #fixed_common_path = re.sub(r'[^-_/1-9a-zA-Z]', '_', common_path)
        print('COMMON PATH %r' % common_path)
        dep.source_path = os.path.join(paths.deps_sources, common_path)
        if not os.path.lexists(dep.source_path):
            tar.extractall(path=paths.deps_sources)

    dep.build_path = os.path.join(paths.deps_build, common_path)

    os.makedirs(dep.source_path, exist_ok=True)
    os.makedirs(dep.build_path, exist_ok=True)


class BootstrapPaths:
    __slots__ = (
        'project',
        'deps',
        'deps_sources',
        'deps_build',
        'deps_prefix',
    )
    
    def items(self):
        for k in BootstrapPaths.__slots__:
            yield k, getattr(self, k, None)


class Dependency:
    __slots__ = (
        'description', 'name', 'version', 'url',
        'source_path', 'build_path', 'archive_path',
    )


if __name__ == '__main__':
    import sys
    main(sys.argv)
else:
    raise RuntimeError('bootstrap.py must be executed as a script')
