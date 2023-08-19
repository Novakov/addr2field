from conans import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class Package(ConanFile):
    name = 'addr2field'
    version = '1.0.0'
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'CMakeToolchain', 'CMakeDeps', 'VirtualBuildEnv'
    requires = (
        'libdwarf/0.7.0@novakov/local',
        'libelf/0.8.13',
        'ms-gsl/4.0.0',
        'spdlog/1.12.0',
    )

    tool_requires = (
        'cmake/3.25.1@',
    )

    scm = {
        'type': 'git',
        'url': 'auto',
        'revision': 'auto',
    }

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def layout(self):
        cmake_layout(self)
