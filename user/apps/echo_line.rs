#![no_std]

#[path = "../lib/rust_support.rs"]
mod rust_support;

const USER_MODE_READY: &[u8] = b"Welcome to echo_line!\n";

#[no_mangle]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32 {
    let mut buf = [0u8; 128];

    let _ = rust_support::write_stdout(USER_MODE_READY);

    loop {
        let count = rust_support::read_stdin(&mut buf);

        if count > 0 {
            if count == 2 && buf[0] == b'$' {
                unsafe {
                    *(0xC0100000 as *mut u8) = 11;
                }
            } else {
                let output_len = count as usize;

                if output_len <= buf.len() {
                    let _ = rust_support::write_stdout_raw(buf.as_ptr(), output_len);
                }
            }
        }
    }
}
