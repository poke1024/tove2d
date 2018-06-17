-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local function linecircle(x1, y1, x2, y2, cx, cy, r)
    -- https://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm

    local dx, dy = x2 - x1, y2 - y1
    local fx, fy = x1 - cx, y1 - cy

    local a = dx * dx + dy * dy
    local b = 2 * (fx * dx + fy * dy)
    local c = fx * fx + fy * fy - r * r

    local discriminant = b * b - 4 * a * c
    local p = {}
    if discriminant >= 0 then
        discriminant = math.sqrt(discriminant)
        local t1 = (-b - discriminant) / (2 * a)
        local t2 = (-b + discriminant) / (2 * a)

        if t1 >= 0 and t1 <= 1 then
            table.insert(p, {x1 + dx * t1, y1 + dy * t1})
        end
        if t2 >= 0 and t2 <= 1 then
            table.insert(p, {x1 + dx * t2, y1 + dy * t2})
        end
    end
    return p
end

local function stripedCircle(cx, cy, outer, inner, phi)
    local g = tove.newGraphics()

    g:drawCircle(cx, cy, outer)
    g:setLineColor(0, 0, 0)
    g:setLineWidth(1)
    g:setFillColor(1, 1, 1)

    if inner > 0 then
        g:closeSubpath()
        g:drawCircle(cx, cy, inner)
        g:invertSubpath()
    end

    g:fill()
    g:stroke()

    local dx = math.cos(phi)
    local dy = math.sin(phi)
    local s = outer / 3
    local sx = -dy * s
    local sy = dx * s
    local i = 0
    local ex, ey = dx * outer * 2, dy * outer * 2
    for i = 0, math.ceil(outer / s) do
        for k = -1, 1, 2 do
            local ax, ay = cx + k * i * sx, cy + k * i * sy
            local p = linecircle(
                ax - ex, ay - ey,
                ax + ex, ay + ey,
                cx, cy, outer)
            if #p == 2 then
                local q = {}
                if inner > 0 then
                    q = linecircle(
                        ax - ex, ay - ey,
                        ax + ex, ay + ey,
                        cx, cy, inner)
                end
                if #q == 2 then
                    g:moveTo(unpack(p[1]))
                    g:lineTo(unpack(q[1]))
                    g:stroke()

                    g:moveTo(unpack(q[2]))
                    g:lineTo(unpack(p[2]))
                    g:stroke()
                else
                    g:moveTo(unpack(p[1]))
                    g:lineTo(unpack(p[2]))
                    g:stroke()
                end
            end
            if i == 0 then
                break
            end
        end
        i = i + 1
    end

    return g
end

return stripedCircle
