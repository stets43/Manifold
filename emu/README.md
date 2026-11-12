# MANIFOLD in the browser

The whole firmware (the app, its UI and its sound) compiled to WebAssembly and run against a simulated FM-1, with
fm1-base's emulator page (`web/emu`, MPL-2.0). Open `index.html` from a web server (not a file:// URL) in a recent
Chrome, Edge, Firefox or Safari and press **Switch on**. It is on the site at https://stets43.github.io/Manifold/emu/.

Built from this repository with fm1-base's `tools/build.py APP --web --release 1.0`, using `tools/emcc-zig/emcc` in
place of Emscripten's `emcc`: it compiles the same C with zig's clang to wasm32-wasi (`pip install ziglang`), linked
as a reactor that exports the emulator's functions. To rebuild:

```
pip install ziglang
export PATH=$PWD/tools/emcc-zig:$PATH          # (from this repository)
cd ../fm1-base && python3 tools/build.py ../Manifold --web --release 1.0
cp ../Manifold/build/emu/{app.wasm,index.html,worklet.js} ../Manifold/emu/
```

With Emscripten installed instead, the same command builds it with the real emcc.
