// imports
extern "C" {
    fn console_log_utf8_from_memory(ptr: *const u8, size: usize);
}

static mut HELLO: [u8; 42] = *b"hello this is something I am trying to say";
#[no_mangle]
pub extern "C" fn say_hello() {
    unsafe {
        let ptr = HELLO.as_ptr();
        console_log_utf8_from_memory(ptr, 42);
    }
}
