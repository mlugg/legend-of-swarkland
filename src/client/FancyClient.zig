client: GameEngineClient,

// puzzle level control stuff
current_level: usize = 0,
starting_level: usize = 0,
moves_per_level: []usize,
pending_forward_actions: usize = 0,
pending_backward_actions: usize = 0,

const std = @import("std");
const core = @import("../index.zig");
const game_server = @import("../server/game_server.zig");
const debugPrintAction = game_server.debugPrintAction;
const Coord = core.geometry.Coord;
const Request = core.protocol.Request;
const NewGameSettings = core.protocol.NewGameSettings;
const Action = core.protocol.Action;
const Response = core.protocol.Response;
const cheatcodes = @import("cheatcodes.zig");
const GameEngineClient = @import("game_engine_client.zig").GameEngineClient;

const the_levels = @import("../server/map_gen.zig").the_levels;
const allocator = std.heap.c_allocator;

pub fn init(client: GameEngineClient) !@This() {
    const moves_per_level = try allocator.alloc(usize, the_levels.len);
    std.mem.set(usize, moves_per_level, 0);
    moves_per_level[0] = 1;

    return @This(){
        .client = client,
        .moves_per_level = moves_per_level,
    };
}

pub fn startGame(self: *@This(), new_game_settings: NewGameSettings) !void {
    try self.enqueueRequest(Request{ .start_game = new_game_settings });
    // starting a new game looks like undoing.
    self.pending_backward_actions += 1;
}

pub fn act(self: *@This(), action: Action) !void {
    try self.enqueueRequest(Request{ .act = action });
    debugPrintAction(0, action);
}
pub fn rewind(self: *@This()) !void {
    const data = self.getMoveCounterInfo();
    if (data.moves_into_current_level == 0 and data.current_level <= self.starting_level) {
        core.debug.move_counter.print("can't undo the start of the starting level", .{});
        return;
    }

    try self.enqueueRequest(.rewind);
    core.debug.actions.print("[rewind]", .{});
}

pub fn move(self: *@This(), direction: Coord) !void {
    return self.act(Action{ .move = direction });
}
pub fn attack(self: *@This(), direction: Coord) !void {
    return self.act(Action{ .attack = direction });
}
pub fn kick(self: *@This(), direction: Coord) !void {
    return self.act(Action{ .kick = direction });
}

pub fn beatLevelMacro(self: *@This(), how_many: usize) !void {
    const data = self.getMoveCounterInfo();
    if (data.moves_into_current_level > 0) {
        core.debug.move_counter.print("beat level is restarting first with {} undos.", .{data.moves_into_current_level});
        for (times(data.moves_into_current_level)) |_| {
            try self.rewind();
        }
    }
    for (times(how_many)) |_, i| {
        if (data.current_level + i >= cheatcodes.beat_level_actions.len) {
            core.debug.move_counter.print("beat level is stopping at the end of the levels.", .{});
            return;
        }
        for (cheatcodes.beat_level_actions[data.current_level + i]) |action| {
            try self.enqueueRequest(Request{ .act = action });
        }
    }
}
pub fn unbeatLevelMacro(self: *@This(), how_many: usize) !void {
    for (times(how_many)) |_| {
        const data = self.getMoveCounterInfo();
        if (data.moves_into_current_level > 0) {
            core.debug.move_counter.print("unbeat level is restarting first with {} undos.", .{data.moves_into_current_level});
            for (times(data.moves_into_current_level)) |_| {
                try self.rewind();
            }
        } else if (data.current_level <= self.starting_level) {
            core.debug.move_counter.print("unbeat level reached the first level.", .{});
            return;
        } else {
            const unbeat_level = data.current_level - 1;
            const moves_counter = self.moves_per_level[unbeat_level];
            core.debug.move_counter.print("unbeat level is undoing level {} with {} undos.", .{ unbeat_level, moves_counter });
            for (times(moves_counter)) |_| {
                try self.rewind();
            }
        }
    }
}

pub fn restartLevel(self: *@This()) !void {
    const data = self.getMoveCounterInfo();
    core.debug.move_counter.print("restarting is undoing {} times.", .{data.moves_into_current_level});
    for (times(data.moves_into_current_level)) |_| {
        try self.rewind();
    }
}

const MoveCounterData = struct {
    current_level: usize,
    moves_into_current_level: usize,
};
fn getMoveCounterInfo(self: @This()) MoveCounterData {
    var remaining_pending_backward_actions = self.pending_backward_actions;
    var remaining_pending_forward_actions = self.pending_forward_actions;
    var pending_current_level = self.current_level;
    while (remaining_pending_backward_actions > self.moves_per_level[pending_current_level] + remaining_pending_forward_actions) {
        core.debug.move_counter.print("before the start of a level: (m[{}]={})+{}-{} < zero,", .{
            pending_current_level,
            self.moves_per_level[pending_current_level],
            remaining_pending_forward_actions,
            remaining_pending_backward_actions,
        });
        remaining_pending_backward_actions -= self.moves_per_level[pending_current_level] + remaining_pending_forward_actions;
        remaining_pending_forward_actions = 0;
        if (pending_current_level == 0) {
            return .{
                .current_level = 0,
                .moves_into_current_level = 0,
            };
        }
        pending_current_level -= 1;
        core.debug.move_counter.print("looking at the previous level.", .{});
    }
    const moves_counter = self.moves_per_level[pending_current_level] + remaining_pending_forward_actions - remaining_pending_backward_actions;
    core.debug.move_counter.print("move counter data: (m[{}]={})+{}-{}={}", .{
        pending_current_level,
        self.moves_per_level[pending_current_level],
        remaining_pending_forward_actions,
        remaining_pending_backward_actions,
        moves_counter,
    });
    return .{
        .current_level = pending_current_level,
        .moves_into_current_level = moves_counter,
    };
}

pub fn stopUndoPastLevel(self: *@This(), starting_level: usize) void {
    self.starting_level = starting_level;
}

fn enqueueRequest(self: *@This(), request: Request) !void {
    switch (request) {
        .act => {
            self.pending_forward_actions += 1;
        },
        .rewind => {
            self.pending_backward_actions += 1;
        },
        .start_game => {},
    }
    try self.client.queues.enqueueRequest(request);
}
pub fn takeResponse(self: *@This()) ?Response {
    const response = self.client.queues.takeResponse() orelse return null;
    // Monitor the responses to count moves per level.
    switch (response) {
        .stuff_happens => |happening| {
            self.pending_forward_actions -= 1;
            self.moves_per_level[happening.frames[0].completed_levels] += 1;
            self.current_level = lastPtr(happening.frames).completed_levels;
        },
        .load_state => |frame| {
            // This can also happen for the initial game load.
            self.pending_backward_actions -= 1;
            self.moves_per_level[frame.completed_levels] -= 1;
            self.current_level = frame.completed_levels;
        },
        .reject_request => |request| {
            switch (request) {
                .act => {
                    self.pending_forward_actions -= 1;
                },
                .rewind => {
                    self.pending_backward_actions -= 1;
                },
                .start_game => {},
            }
        },
    }
    return response;
}

fn lastPtr(arr: anytype) @TypeOf(&arr[arr.len - 1]) {
    return &arr[arr.len - 1];
}

fn times(n: usize) []const void {
    return @as([(1 << (@sizeOf(usize) * 8) - 1)]void, undefined)[0..n];
}
