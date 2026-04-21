# Rust User Apps

User apps in `user/apps` can now be written in Rust as `*.rs` files. They are built into the same embedded ELF format as the existing C apps, so the kernel loader path does not change.

## Build requirements

If the tree contains Rust user apps, `build.sh` requires:

- `rustc`
- a Rust target that can emit 32-bit ELF objects

By default the build uses:

- `USER_RUST_TARGET=i686-unknown-linux-gnu`

You can override that target when building:

```sh
USER_RUST_TARGET=i686-unknown-linux-gnu ./build.sh --build-only
```

## App shape

A Rust user app should:

- use `#![no_std]`
- export `main` with the same ABI the existing user entry stub expects
- optionally include the helper module at `user/lib/rust_support.rs`

Example:

```rust
#![no_std]

#[path = "../lib/rust_support.rs"]
mod rust_support;

#[unsafe(no_mangle)]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32 {
    let _ = rust_support::write_stdout(b"Hello from Rust user space!\n");

    loop {}
}
```

## Notes

- The kernel still enters user space through `user_program_entry` from `user/lib/entry_stub.S`.
- Rust apps do not replace that entry path; they only provide the `main` symbol it calls.
- The helper module exposes thin wrappers for `read`, `write`, and `execve`, plus a panic handler.
