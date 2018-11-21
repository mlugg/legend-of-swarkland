const std = @import("std");
const sdl = @import("./sdl.zig");
const textures = @import("./textures.zig");
const gui = @import("./gui.zig");
const core = @import("core");
const makeCoord = core.geometry.makeCoord;
const GameEngine = core.game_engine_client.GameEngine;

const GameState = enum {
    MainMenu,
    Running,
};

pub fn main() anyerror!void {
    core.debug.prefix_name = "client";
    core.debug.warn("init\n");

    if (sdl.c.SDL_Init(sdl.c.SDL_INIT_VIDEO) != 0) {
        std.debug.panic("SDL_Init failed: {c}\n", sdl.c.SDL_GetError());
    }
    defer sdl.c.SDL_Quit();

    const screen = sdl.c.SDL_CreateWindow(
        c"Legend of Swarkland",
        sdl.SDL_WINDOWPOS_UNDEFINED,
        sdl.SDL_WINDOWPOS_UNDEFINED,
        512,
        512,
        sdl.c.SDL_WINDOW_OPENGL | sdl.c.SDL_WINDOW_RESIZABLE,
    ) orelse {
        std.debug.panic("SDL_CreateWindow failed: {c}\n", sdl.c.SDL_GetError());
    };
    defer sdl.c.SDL_DestroyWindow(screen);

    const renderer: *sdl.Renderer = sdl.c.SDL_CreateRenderer(screen, -1, 0) orelse {
        std.debug.panic("SDL_CreateRenderer failed: {c}\n", sdl.c.SDL_GetError());
    };
    defer sdl.c.SDL_DestroyRenderer(renderer);

    textures.init(renderer);
    defer textures.deinit();

    try doMainLoop(renderer);
}

fn doMainLoop(renderer: *sdl.Renderer) !void {
    var game_state = GameState.MainMenu;
    var main_menu_state = gui.LinearMenuState.init();
    var game_engine: ?GameEngine = null;
    defer if (game_engine) |*g| g.stopEngine();

    while (true) {
        const now = sdl.c.SDL_GetTicks();
        if (game_engine) |*g| {
            g.pollEvents(now);
        }

        main_menu_state.beginFrame();
        var event: sdl.c.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event) != 0) {
            switch (event.@"type") {
                sdl.c.SDL_QUIT => {
                    return;
                },
                sdl.c.SDL_KEYDOWN => {
                    switch (game_state) {
                        GameState.MainMenu => {
                            switch (@enumToInt(event.key.keysym.scancode)) {
                                sdl.c.SDL_SCANCODE_UP => {
                                    main_menu_state.moveUp();
                                },
                                sdl.c.SDL_SCANCODE_DOWN => {
                                    main_menu_state.moveDown();
                                },
                                sdl.c.SDL_SCANCODE_RETURN => {
                                    main_menu_state.enter();
                                },
                                else => {},
                            }
                        },
                        GameState.Running => {
                            switch (@enumToInt(event.key.keysym.scancode)) {
                                sdl.c.SDL_SCANCODE_LEFT => {
                                    try game_engine.?.move(makeCoord(-1, 0));
                                },
                                sdl.c.SDL_SCANCODE_RIGHT => {
                                    try game_engine.?.move(makeCoord(1, 0));
                                },
                                sdl.c.SDL_SCANCODE_UP => {
                                    try game_engine.?.move(makeCoord(0, -1));
                                },
                                sdl.c.SDL_SCANCODE_DOWN => {
                                    try game_engine.?.move(makeCoord(0, 1));
                                },
                                sdl.c.SDL_SCANCODE_BACKSPACE => {
                                    try game_engine.?.rewind();
                                },
                                else => {},
                            }
                        },
                    }
                },
                else => {},
            }
        }

        _ = sdl.c.SDL_RenderClear(renderer);

        switch (game_state) {
            GameState.MainMenu => {
                var menu_renderer = gui.Gui.init(renderer, &main_menu_state, textures.sprites.shiny_purple_wand);

                menu_renderer.seek(10, 10);
                menu_renderer.scale(2);
                menu_renderer.bold(true);
                menu_renderer.marginBottom(5);
                menu_renderer.text("Legend of Swarkland");
                menu_renderer.scale(1);
                menu_renderer.bold(false);
                menu_renderer.seekRelative(70, 0);
                if (menu_renderer.button("New Game")) {
                    game_state = GameState.Running;
                    game_engine = GameEngine(undefined);
                    try game_engine.?.startEngine();
                }
                if (menu_renderer.button("Load Yagni")) {
                    // actually quit
                    return;
                }
                if (menu_renderer.button("Quit")) {
                    // quit
                    return;
                }
            },
            GameState.Running => {
                const g = &game_engine.?;
                var display_position = g.game_state.position.scaled(32);
                if (g.position_animation) |animation| blk: {
                    const duration = @intCast(i32, animation.end_time - animation.start_time);
                    const progress = @intCast(i32, now - animation.start_time);
                    if (progress > duration) {
                        g.position_animation = null;
                        break :blk;
                    }
                    const vector = animation.to.minus(animation.from).scaled(32);
                    display_position = animation.from.scaled(32).plus(vector.scaled(progress).scaledDivTrunc(duration));
                }
                textures.renderSprite(renderer, textures.sprites.human, display_position.plus(makeCoord(128, 128)));
            },
        }

        sdl.c.SDL_RenderPresent(renderer);

        // delay until the next multiple of 17 milliseconds
        const delay_millis = 17 - (sdl.c.SDL_GetTicks() % 17);
        sdl.c.SDL_Delay(delay_millis);
    }
}
