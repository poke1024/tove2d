#!python

AddOption(
	'--tovedebug',
	action='store_true',
	help='debug build',
	default=False)

sources = [
	"src/cpp/version.cpp",
	"src/cpp/binding.cpp",
	"src/cpp/graphics.cpp",
	"src/cpp/nsvg.cpp",
	"src/cpp/paint.cpp",
	"src/cpp/path.cpp",
	"src/cpp/subpath.cpp",
	"src/cpp/utils.cpp",
	"src/cpp/mesh/flatten.cpp",
	"src/cpp/mesh/mesh.cpp",
	"src/cpp/mesh/meshifier.cpp",
	"src/cpp/mesh/utils.cpp",
	"src/cpp/shader/curvedata.cpp",
	"src/cpp/shader/data.cpp",
	"src/cpp/shader/geometry.cpp",
	"src/thirdparty/clipper.cpp",
	"src/thirdparty/polypartition.cpp",
	"src/cpp/warn.cpp"]

env = Environment()

if env["PLATFORM"] == 'win32':
	env["CCFLAGS"] = ' /EHsc '
	if not GetOption('tovedebug'):
		env["CCFLAGS"] += ' /O1 /Oy /Oi /fp:fast '
		env["LINKFLAGS"] = ' /OPT:REF '
else:
	CCFLAGS = ' -std=c++11 -fvisibility=hidden -funsafe-math-optimizations '

	if GetOption('tovedebug'):
		CCFLAGS += '-g '
	else:
		CCFLAGS += '-O2 '

	if env["PLATFORM"] == 'posix':
		CCFLAGS += ' -mf16c '

	env["CCFLAGS"] = CCFLAGS

lib = env.SharedLibrary(target='lib/libTove', source=sources)

# prepare git hash based version string.

import subprocess
import datetime
import re
import os

def get_version_string():
	os.chdir(Dir('.').abspath)
	args = ['git', 'describe', '--tags', '--long']
	return subprocess.check_output(args).strip()

with open("src/cpp/version.in.cpp", "r") as v_in:
	with open("src/cpp/version.cpp", "w") as v_out:
		v_out.write(v_in.read().replace("TOVE_VERSION", get_version_string()))

# now glue together the lua library.

def minify_lua(target, source, env):
	include_pattern = re.compile("\s*--!!\s*include\s*\"([^\"]+)\"\s*")
	import_pattern = re.compile("\s*--!!\s*import\s*\"([^\"]+)\"\s*as\s*(\w+)\s*")

	def lua_import(path):
		source = []

		basepath, _ = os.path.split(path)
		filename, ext = os.path.splitext(path)

		with open(path) as f:
			lines = f.readlines()

		for line in lines:
			m = include_pattern.match(line)
			if m:
				included_path = os.path.join(basepath, m.group(1))
				source.extend(lua_import(included_path))
				continue

			m = import_pattern.match(line)
			if m:
				imported_path = os.path.join(basepath, m.group(1))
				source.append("local %s = (function()\n" % m.group(2))
				source.extend(lua_import(imported_path))
				source.append("end)()\n")
				continue

			if ext == ".lua" and (line.strip().startswith("-- ") or line.strip() == "--"):
				line = None

			if ext == ".h":
				line = re.sub("^EXPORT ", "", line)

			if line:
				source.append(line)

		return source

	assert len(target) == 1
	with open(target[0].abspath, "w") as f:
		f.write("--  This file has been autogenerated. DO NOT CHANGE IT.\n--\n")
		for s in source:
			if os.path.split(s.abspath)[1] == "main.lua":
				f.write("".join(lua_import(s.abspath)))

env.Append(BUILDERS={'MinifyLua':Builder(action=minify_lua)})
env.MinifyLua('lib/tove.lua',
	Glob("src/cpp/_interface.h") +
	Glob("src/lua/*.lua") +
	Glob("src/lua/core/*.lua") +
	Glob("src/lua/core/*.glsl"))
