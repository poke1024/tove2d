-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow

local graphics = {nil, nil, nil}
local algorithms = {"bayer8", "bayer4", "bayer2", "floyd", "atkinson", "jarvis", "stucki"}
local algorithmIndex = #algorithms
local spread = 1
local noise = 0.01

-- a 33 color sample palette by https://twitter.com/Fleja2003
local colors = {
	"#1f1833",
	"#2b2e42",
	"#414859",
	"#68717a",
	"#90a1a8",
	"#b6cbcf",
	"#ffffff",
	"#fcbf8a",
	"#b58057",
	"#8a503e",
	"#5c3a41",
	"#c93038",
	"#de6a38",
	"#ffad3b",
	"#ffe596",
	"#fcf960",
	"#b4d645",
	"#51c43f",
	"#309c63",
	"#236d7a",
	"#264f6e",
	"#233663",
	"#417291",
	"#4c93ad",
	"#63c2c9",
	"#94d2d4",
	"#b8fdff",
	"#3c2940",
	"#46275c",
	"#826481",
	"#f7a48b",
	"#c27182",
	"#852d66"
}

local function updateGraphics()
	local algo = algorithms[algorithmIndex]
	graphics[1]:setDisplay("texture", "retro", colors, algo, spread, noise)
	graphics[2]:setDisplay("texture", "retro", "pico8", algo, spread, noise)
	graphics[3]:setDisplay("texture", "retro", "bw", algo, spread, noise)
end

local function load()
	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()

	local svg = love.filesystem.read("assets/monster_by_mike_mac.svg")

	graphics[1] = tove.newGraphics(svg, 200)
	flow:add("Fleja2003", graphics[1])

	graphics[2] = tove.newGraphics(svg, 200)
	flow:add("PICO8", graphics[2])

	graphics[3] = tove.newGraphics(svg, 200)
	flow:add("B/W", graphics[3])

	updateGraphics()
end

load()
love.keyboard.setKeyRepeat(true)

function love.draw()
	tovedemo.draw("Retro Rasterization.",
		"\n[a]lgorithm: " .. algorithms[algorithmIndex] ..
		"\ncolor [s]pread: " .. string.format("%.2f", spread) ..
		"\n[n]oise: " .. string.format("%.3f", noise))
	tovedemo.attribution("Graphics by Mike Mac.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end

function love.keypressed(key, scancode)
	local shift = love.keyboard.isScancodeDown('lshift') or love.keyboard.isScancodeDown('rshift')
	if scancode == 's' then
		if shift then
			spread = math.max(spread / 1.5, 0.1)
		else
			spread = math.min(spread * 1.5, 25.0)
		end
	end
	if key == 'a' then
		if shift then
			algorithmIndex = algorithmIndex - 1
			if algorithmIndex == 0 then
				algorithmIndex = #algorithms
			end
		else
			algorithmIndex = algorithmIndex + 1
			if algorithmIndex > #algorithms then
				algorithmIndex = 1
			end
		end
	end
	if key == 'n' then
		if shift then
			noise = math.max(noise / 1.5, 0.01)
		else
			noise = math.min(noise * 1.5, 0.2)
		end
	end
	updateGraphics()
end
