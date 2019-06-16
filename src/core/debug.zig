const std = @import("std");
pub fn warn(comptime fmt: []const u8, args: ...) void {
    // format to a buffer, then write in a single (or as few as possible)
    // posix write calls so that the output from multiple processes
    // doesn't interleave on the same line.
    var buffer: [0x1000]u8 = undefined;
    const debug_thread_id = blk: {
        const me = std.Thread.getCurrentId();
        for (thread_names) |*maybe_it| {
            if (maybe_it.*) |it| {
                if (it.thread_id == me) break :blk it;
            }
        }
        @panic("thread not named");
    };
    std.debug.warn("{}", std.fmt.bufPrint(buffer[0..], "{}({}): " ++ fmt ++ "\n", debug_thread_id.name, debug_thread_id.client_id, args) catch {
        @panic("make the buffer bigger");
    });
}

var mutex: ?std.Mutex = null;
pub fn init() void {
    mutex = std.Mutex.init();
}

const DebugThreadId = struct {
    thread_id: std.Thread.Id,
    name: []const u8,
    client_id: u32,
};
var thread_names = [_]?DebugThreadId{null} ** 100;
pub fn nameThisThread(name: []const u8) void {
    return nameThisThreadWithClientId(name, 0);
}
pub fn nameThisThreadWithClientId(name: []const u8, client_id: u32) void {
    var held = mutex.?.acquire();
    defer held.release();
    const thread_id = std.Thread.getCurrentId();
    for (thread_names) |*maybe_it| {
        if (maybe_it.*) |it| {
            std.debug.assert(it.thread_id != thread_id);
            std.debug.assert(!(std.mem.eql(u8, it.name, name) and it.client_id == client_id));
            continue;
        }
        maybe_it.* = DebugThreadId{
            .thread_id = thread_id,
            .name = name,
            .client_id = client_id,
        };
        return;
    }
    @panic("too many threads");
}

/// i kinda wish std.fmt did this.
pub fn deep_print(prefix: []const u8, something: var) void {
    std.debug.warn("{}", prefix);
    struct {
        pub fn recurse(obj: var, comptime indent: comptime_int) void {
            const T = @typeOf(obj);
            const indentation = ("  " ** indent)[0..];
            if (comptime std.mem.startsWith(u8, @typeName(T), "std.array_list.AlignedArrayList(")) {
                return recurse(obj.toSliceConst(), indent);
            }
            if (comptime std.mem.startsWith(u8, @typeName(T), "std.hash_map.HashMap(u32,")) {
                if (obj.count() == 0) {
                    return std.debug.warn("{{}}");
                }
                std.debug.warn("{{");
                var iterator = obj.iterator();
                while (iterator.next()) |kv| {
                    std.debug.warn("\n{}  {}: ", indentation, kv.key);
                    recurse(kv.value, indent + 1);
                    std.debug.warn(",");
                }
                return std.debug.warn("\n{}}}", indentation);
            }
            switch (@typeInfo(T)) {
                .Pointer => |ptr_info| switch (ptr_info.size) {
                    .One => return recurse(obj.*, indent),
                    .Slice => {
                        if (obj.len == 0) {
                            return std.debug.warn("[]");
                        }
                        std.debug.warn("[\n");
                        for (obj) |x| {
                            std.debug.warn("{}  ", indentation);
                            recurse(x, indent + 1);
                            std.debug.warn(",\n");
                        }
                        return std.debug.warn("{}]", indentation);
                    },
                    else => {},
                },
                .Array => {
                    return recurse(obj[0..], indent);
                },
                .Struct => {
                    const multiline = @sizeOf(T) >= 12;
                    comptime var field_i = 0;
                    std.debug.warn("{{");
                    inline while (field_i < @memberCount(T)) : (field_i += 1) {
                        if (field_i > 0) {
                            if (!multiline) {
                                std.debug.warn(", ");
                            }
                        } else if (!multiline) {
                            std.debug.warn(" ");
                        }
                        if (multiline) {
                            std.debug.warn("\n{}  ", indentation);
                        }
                        std.debug.warn(".{} = ", @memberName(T, field_i));
                        recurse(@field(obj, @memberName(T, field_i)), indent + 1);
                        if (multiline) std.debug.warn(",");
                    }
                    if (multiline) {
                        std.debug.warn("\n{}}}", indentation);
                    } else {
                        std.debug.warn(" }}");
                    }
                    return;
                },
                .Union => |info| {
                    if (info.tag_type) |UnionTagType| {
                        std.debug.warn("{}.{} = ", @typeName(T), @tagName(UnionTagType(obj)));
                        inline for (info.fields) |u_field| {
                            if (@enumToInt(UnionTagType(obj)) == u_field.enum_field.?.value) {
                                recurse(@field(obj, u_field.name), indent);
                            }
                        }
                        return;
                    }
                },
                .Enum => |info| {
                    return std.debug.warn(".{}", @tagName(obj));
                },
                else => {},
            }
            return std.debug.warn("{}", obj);
        }
    }.recurse(something, 0);
    std.debug.warn("\n");
}
