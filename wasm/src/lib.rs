// imports
extern "C" {
    fn console_log_bytes(ptr: *const u8, size: usize);
    fn console_log_utf8(ptr: *const u8, size: usize);
    fn get_canvas_width() -> f32;
    fn get_canvas_height() -> f32;
}

unsafe fn console_log(log: &str) {
    console_log_utf8(log.as_ptr(), log.len());
}

// GLOBALS

const BUFFER_SIZE: usize = 1024;

// BUFFERS
static mut COLOR_SHADER_VERTICES: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
static mut COLOR_SHADER_COLORS: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
static mut COLOR_SHADER_ENTITIES_COUNT: u32 = 0;


static mut COLOR_SHADER_VERTICES_ARENA: MemoryArena = MemoryArena {
    data:  unsafe {&mut COLOR_SHADER_VERTICES},
    current_pos: 0,
    size: BUFFER_SIZE,
};

static mut COLOR_SHADER_COLORS_ARENA: MemoryArena = MemoryArena {
    data:  unsafe {&mut COLOR_SHADER_COLORS},
    current_pos: 0,
    size: BUFFER_SIZE,
};


static mut GAME_STATE: GameState = GameState {
    player_x: 0.0,
    player_y: 0.0,
    is_initialized: false,
    t_last_frame: 0.0,
};


// TYPES
struct GameCanvas {
    width: f32,
    height: f32,
}

struct MemoryArena {
    data: &'static mut [u8],
    current_pos: usize,
    size: usize,
}

struct Color {
    r: f32,
    g: f32,
    b: f32,
    a: f32,
}

struct GameState {
    is_initialized: bool,

    t_last_frame: f64, // DOMHighResTimestamp is a double

    player_x: f32,
    player_y: f32,
}

struct Keyboard {
    key_a: bool,
    key_s: bool,

    key_up: bool,
    key_down: bool,
    key_right: bool,
    key_left: bool,
}


// EXPORTS
#[no_mangle]
 pub extern "C" fn game_update_and_render(t_frame: f64) {
    unsafe {
        // clear memory arenas that contain draw data for each frame
        reset_arena(&mut COLOR_SHADER_VERTICES_ARENA);
        reset_arena(&mut COLOR_SHADER_COLORS_ARENA);

        let canvas = GameCanvas {
            width: get_canvas_width(),
            height: get_canvas_height(),
        };

        if !GAME_STATE.is_initialized {
            GAME_STATE.is_initialized = true;
            GAME_STATE.player_x = 1.0;
            GAME_STATE.player_y = 0.1;
            GAME_STATE.t_last_frame = t_frame;
        }

        let dt: f64 = t_frame - GAME_STATE.t_last_frame;
        GAME_STATE.t_last_frame = t_frame;

        //console_log_bytes(t_frame.to_be_bytes().as_ptr(), 8);

        draw_rectangle(0.0, 0.0, canvas.width, canvas.height, Color{ r: 1.0, g: 0.0, b: 1.0, a: 1.0});

        // Draw player
        let player_width_meters: f32 = 0.9;
        let player_height_meters: f32 = 1.8;

        let pixels_per_meter: f32 = 30.0;
        let player_width_pixels = player_width_meters * pixels_per_meter;
        let player_height_pixels = player_height_meters * pixels_per_meter;

        let canvas_player_x = GAME_STATE.player_x * pixels_per_meter;
        let canvas_player_y = canvas.height - GAME_STATE.player_y * pixels_per_meter;
        draw_rectangle(
            canvas_player_x - (player_width_pixels / 2.0),
            canvas_player_y - player_height_pixels,
            canvas_player_x + (player_width_pixels / 2.0),
            canvas_player_y,
            Color { r: 0.8, g: 0.6, b: 0.2, a: 1.0}
        );
    }
}


#[no_mangle]
pub extern "C" fn color_shader_vertices_pointer() -> *const u8 {
    let pointer: *const u8;
    unsafe {
        pointer = COLOR_SHADER_VERTICES_ARENA.data.as_ptr() as *const u8;
    }
    pointer
}

#[no_mangle]
pub extern "C" fn color_shader_colors_pointer() -> *const u8 {
    let pointer: *const u8;
    unsafe {
        pointer = COLOR_SHADER_COLORS_ARENA.data.as_ptr() as *const u8;
    }
    pointer
}

#[no_mangle]
pub extern "C" fn color_shader_entities_count() -> u32 {
    unsafe {
        COLOR_SHADER_ENTITIES_COUNT
    }
}


unsafe fn draw_rectangle(min_x: f32, min_y: f32, max_x: f32, max_y: f32, color: Color) {
    // TODO: Maybe just memcpy the whole array?
    // Left Triangle
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_y);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_y);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_y);

    // Right Triangle
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_y);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, min_y);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_x);
    push_f32_to_buffer(&mut COLOR_SHADER_VERTICES_ARENA, max_y);

    // Color
    push_f32_to_buffer(&mut COLOR_SHADER_COLORS_ARENA, color.r);
    push_f32_to_buffer(&mut COLOR_SHADER_COLORS_ARENA, color.g);
    push_f32_to_buffer(&mut COLOR_SHADER_COLORS_ARENA, color.b);
    push_f32_to_buffer(&mut COLOR_SHADER_COLORS_ARENA, color.a);

    COLOR_SHADER_ENTITIES_COUNT += 1;
}

unsafe fn push_byte_to_buffer(arena: &mut MemoryArena, byte: u8) {
    if arena.current_pos >= arena.size - 1 {
        console_log("Memory buffer full");
        panic!()
    }

    arena.data[arena.current_pos] = byte;
    arena.current_pos += 1;
}

fn push_f32_to_buffer(arena: &mut MemoryArena, value: f32) {
    // TODO: Check endianess of the machine somehow and convert accordingly
    // DataView does not seem to work if passing the full array
    let bytes = value.to_ne_bytes();
    for byte in bytes.iter() {
        unsafe {
            push_byte_to_buffer(arena, *byte);
        }
    }
}

fn reset_arena(arena: &mut MemoryArena) {
    arena.current_pos = 0;
}

