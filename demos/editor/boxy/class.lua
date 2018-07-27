return setmetatable({
		is_a = function(instance, Class)
			local mt = getmetatable(instance)
			repeat
				if mt == Class then
					return true
				end
				mt = getmetatable(mt).__index -- base
			until mt == nil
			return false
		end
	}, {
		__call = function(_, name, base)
			local Class = {__name = name}
			Class.__index = Class
			return setmetatable(Class, {
				__index = base,
				__call = function(Class, ...)
					local instance = setmetatable({}, Class)
					local init = Class.__init
					if init then
						init(instance, ...)
					end
					return instance
				end
			})
		end
	})