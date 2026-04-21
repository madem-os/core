#![no_std]

#[path = "../lib/rust_support.rs"]
mod rust_support;

const USER_MODE_READY: &[u8] = b"Welcome to shell!\n";
const ECHO_LINE_PROGRAM: &[u8] = b"echo_line\0";

#[no_mangle]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32 {
    let mut buf = [0u8; 128];

    let _ = rust_support::write_stdout(USER_MODE_READY);

    loop {
        let count = rust_support::read_stdin(&mut buf);

        if count > 0 {
            rust_support::exec(ECHO_LINE_PROGRAM.as_ptr());
        }
    }
}
