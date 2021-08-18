// imports
extern "C" {
    fn console_log_bytes(ptr: *const u8, size: usize);
    fn console_log_utf8(ptr: *const u8, size: usize);
}

unsafe fn console_log(log: &str) {
    console_log_utf8(log.as_ptr(), log.len());
}

// TODO: Can this be a struct?
const MEMORY_BUFFER_SIZE: usize = 1024;
static mut MEMORY_BUFFER: [u8; MEMORY_BUFFER_SIZE] = [1; MEMORY_BUFFER_SIZE];

struct MemoryArena {
    data: &'static mut [u8],
    current_pos: usize,
    size: usize,
}

static mut ARENA: MemoryArena = MemoryArena {
    data:  unsafe {&mut MEMORY_BUFFER},
    current_pos: 0,
    size: MEMORY_BUFFER_SIZE,
};


#[no_mangle]
pub extern "C" fn write_vertices() {
    push_f32_to_buffer(20_f32);
    push_f32_to_buffer(10_f32);

    push_f32_to_buffer(0_f32);
    push_f32_to_buffer(50_f32);

    push_f32_to_buffer(50_f32);
    push_f32_to_buffer(60_f32);
}

// TODO: Pass ARENA as argument
unsafe fn push_byte_to_buffer(byte: u8) {
    if ARENA.current_pos >= ARENA.size - 1 {
        console_log("Memory buffer full");
        panic!()
    }

    ARENA.data[ARENA.current_pos] = byte;
    ARENA.current_pos += 1;
}

fn push_f32_to_buffer(value: f32) {
    // TODO: Check endianess of the machine somehow and convert accordingly
    let bytes = value.to_le_bytes();
    for byte in bytes.iter() {
        unsafe {
            push_byte_to_buffer(*byte);
        }
    }
}

#[no_mangle]
pub extern "C" fn buffer_pointer() -> *const u8 {
    let pointer: *const u8;
    unsafe {
        pointer = ARENA.data.as_ptr() as *const u8;
    }
    pointer
}
