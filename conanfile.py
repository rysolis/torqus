from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class TorqusConan(ConanFile):
    name = "torqus"
    version = "0.1.0"
    license = "Apache-2.0"
    url = "https://github.com/open-ppv/torqus"
    description = "A C++20 TFHE library (leveled arithmetic, gate bootstrapping)"
    topics = ("tfhe", "fhe", "homomorphic-encryption", "cryptography")

    settings = "os", "arch", "compiler", "build_type"
    package_type = "header-library"
    no_copy_source = True

    options = {
        "use_mimalloc": [True, False],
        "enable_simd": [True, False],
        "enable_noise": [True, False],
    }
    default_options = {
        "use_mimalloc": True,
        "enable_simd": True,
        "enable_noise": True,
    }

    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "LICENSE"

    def requirements(self):
        # Optional even in the CMakeLists.txt sense (falls back to the
        # system allocator if not found) -- only actually required() here
        # because Conan needs to resolve/build it up front when the option
        # is on, unlike CMake's own find_package(... QUIET) probe.
        if self.options.use_mimalloc:
            self.requires("mimalloc/2.1.9")

    def package_id(self):
        # Header-only, so settings (os/arch/compiler/build_type) don't
        # affect the package -- nothing here gets compiled. Options are a
        # different story: use_mimalloc is baked into the installed
        # torqusConfig.cmake's find guard, and enable_simd/enable_noise
        # become INTERFACE compile definitions exported via
        # torqusTargets.cmake, so different option values really do
        # produce different installed package content and must stay part
        # of the package_id.
        self.info.settings.clear()

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTING"] = False
        tc.variables["TORQUS_USE_MIMALLOC"] = bool(self.options.use_mimalloc)
        tc.variables["TORQUS_ENABLE_SIMD"] = bool(self.options.enable_simd)
        tc.variables["TFHE_ENABLE_NOISE"] = bool(self.options.enable_noise)
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "torqus")
        self.cpp_info.set_property("cmake_target_name", "torqus::torqus")
        self.cpp_info.libdirs = []
        self.cpp_info.bindirs = []
