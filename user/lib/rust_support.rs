use core::panic::PanicInfo;

unsafe extern "C" {
    fn read(fd: i32, buf: *mut u8, len: i32) -> i32;
    fn write(fd: i32, buf: *const u8, len: i32) -> i32;
    fn execve(program_name: *const u8);
}

pub fn read_stdin(buf: &mut [u8]) -> i32 {
    unsafe { read(0, buf.as_mut_ptr(), buf.len() as i32) }
}

pub fn write_stdout(buf: &[u8]) -> i32 {
    unsafe { write(1, buf.as_ptr(), buf.len() as i32) }
}

pub fn write_stdout_raw(ptr: *const u8, len: usize) -> i32 {
    unsafe { write(1, ptr, len as i32) }
}

pub fn exec(program_name: *const u8) -> ! {
    unsafe {
        execve(program_name);
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    let _ = write_stdout(b"Rust panic\n");

    loop {}
}
