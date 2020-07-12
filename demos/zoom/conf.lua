function love.conf(t)
	-- setting msaa is important to get antialiased
	-- results for the mesh renderer. you should not
	-- set this if you only use gpux renderers.

	t.window.msaa = 2
end
