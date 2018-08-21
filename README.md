# Render Pipeline C++

This project is C++ ported version of [Render Pipeline](https://github.com/tobspr/RenderPipeline).

And I am developing the project and new features for CR Software Framework (...).

#### Build Status
| OS       | Build Status           | Latest Build                                                                                |
| :------: | :--------------------: | :-----------------------------------------------------------------------------------------: |
| Windows  | [![ci-badge]][ci-link] | vc140 ([Release][vc140-release])<br/>vc141 ([Debug][vc141-debug], [Release][vc141-release]) |

[ci-badge]: https://ci.appveyor.com/api/projects/status/uo5j9rd751aux6l1/branch/master?svg=true "AppVeyor build status"
[ci-link]: https://ci.appveyor.com/project/bluekyu/render-pipeline-cpp/branch/master "AppVeyor build link"
[vc140-release]: https://ci.appveyor.com/api/projects/bluekyu/render-pipeline-cpp/artifacts/render_pipeline_cpp.7z?branch=master&job=Image%3A+Visual+Studio+2015%3B+Configuration%3A+Release "Download latest vc140 build (Release)"
[vc141-debug]: https://ci.appveyor.com/api/projects/bluekyu/render-pipeline-cpp/artifacts/render_pipeline_cpp.7z?branch=master&job=Image%3A+Visual+Studio+2017%3B+Configuration%3A+Debug "Download latest vc141 build (Debug)"
[vc141-release]: https://ci.appveyor.com/api/projects/bluekyu/render-pipeline-cpp/artifacts/render_pipeline_cpp.7z?branch=master&job=Image%3A+Visual+Studio+2017%3B+Configuration%3A+Release "Download latest vc141 build (Release)"

**Note**: These builds are built with *patched* [Panda3D](https://github.com/bluekyu/panda3d).



## Documents
See [docs/index.md](docs/index.md) document.



## Library Requirements
- Panda3D
- Boost

### Optional Third-party
These are required when you include related headers to access internal data.

- [yaml-cpp](https://github.com/jbeder/yaml-cpp): required when to access YAML node.
- [spdlog](https://github.com/gabime/spdlog): required when to access the internal logger of spdlog.
- [assimp](https://github.com/assimp/assimp): required to use rpassimp plugin.



## Tested Platforms
I tested it in the following platforms:
- Intel CPU, NVIDIA GPU, Windows 10 64-bit, Visual Studio 2017
- Intel CPU, Intel GPU (UHD Graphics 620), Windows 10 64-bit, Visual Studio 2017

Visual Studio 2015 may be able to compile it, but I did not test.


## Build
See [docs/build_rpcpp.md](docs/build_rpcpp.md) document.



## Related Projects
- Panda3D Third-party: https://github.com/bluekyu/panda3d-thirdparty
- (patched) Panda3D: https://github.com/bluekyu/panda3d
- Plugins for Render Pipeline C++: https://github.com/bluekyu/rpcpp_plugins
- Samples and snippets for Render Pipeline C++: https://github.com/bluekyu/rpcpp_samples



## License
See `LICENSE.md` file.

And license of original (python based) [Render Pipeline](https://github.com/tobspr/RenderPipeline)
is `LICENSE-RenderPipeline.md` file in `thirdparty-licenses` directory.

### Third-party Licenses
See `thirdparty-licenses` directory.
