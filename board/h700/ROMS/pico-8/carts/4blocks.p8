pico-8 cartridge // http://www.pico-8.com
version 42
__lua__
-- constants -------------------

--[[ the number of lines to
clear in the second game type ]]
b_lines = 25

--[[ the line height for the
third game type ]]
c_line_height = 6

--[[ the gravity of each level
is determined by the curve:
g = (.0013 * lv^2.015) + .04
0=no speed 1=move every frame ]]
lv_gravity = { .040,.041,.045,
.052,.061,.073,.088,.106,.126,
.149,.175,.203,.234,.268,.305,
.345,.387,.432,.480,.530,.584 }

--[[ the points for each number
of lines cleared. this is:
points = flr(score_x_mul *
         	   gravity)
also, a "0" is added to the end
of the score string. ]]
score_1_mul = 80
score_2_mul = 200
score_3_mul = 600
score_4_mul = 2000

--[[ if a line has been
cleared right after another
line clear, multiply the points
by this bonus. ]]
consec_lin_mul = 1.2

--[[ when left or right is held
down, this is how many frames to
wait before moving
automatically ]]
move_delay = 5

--[[ the speed to move the piece
when left or right is held down
0 = no speed
1 = move every frame ]]
move_speed = 0.33

--[[ the speed to move the piece
when down is held down
0 = no speed
1 = move every frame ]]
down_speed = 0.75

--[[ the number of lines to
clear to go to the next level ]]
lines_to_next_lv = 10

--[[ the number of lines to
clear in the starting level to
go to the next level is:
(start_level * start_lv_mul) +
 lin_to_next_lv ]]
start_lv_mul = 5

--[[ for b game type, the
minimum and maximun number of
blocks that are generated in
each row ]]
min_b_bl = 3
max_b_bl = 8

--[[ the speed of the
background in px/frame ]]
back_speed = 1

--[[ the speed of the game
over animation in px/frame ]]
go_anim_speed = 4

--[[ the size of the bag, which
is a randomly generated sequence
of pieces. this way, the number
of times each piece appears can
be more balanced. this cannot
be larger than
max_appear * 7 ]]
rand_bag_size = 18

--[[ the maximum number of times
the same piece can appear
in a bag ]]
max_appear = 3

--[[ the maximum seperation
between 2 line pieces ]]
max_lin_sep = 16

--[[ the maximum number of times
an "s" and "z" piece can appear
in a row ]]
max_sz_seq = 4

-- the version of this cart
version = "v1.1"

-- main functions --------------

function _init()
	init_grid()
	init_p_bl()
	init_rand_bag()
	init_next_piece()
	init_place_anim()
	
	load_save_data()
	set_highscore_a(highscore_a)
	set_highscore_b(highscore_b)
	set_highscore_c(highscore_c)
	init_menu_items()
	
	start_start_screen()
end

function _update()
	if(global_state == "gm") then
		update_game()
	elseif(global_state=="mm") then
		update_main_menu()
	end
end

function _draw()
	if(global_state == "gm") then
		draw_game()
	elseif(global_state=="mm") then
		draw_main_menu()
	else
		draw_start_screen()
	end
end

-- global arrays ---------------

-- the grid of blocks
grid = {}
function init_grid()
	for ix=1,10 do
		grid[ix] = {}
		for iy=1,20 do
			grid[ix][iy] = {}
			grid[ix][iy].bl = false
			grid[ix][iy].sp = 0
		end
	end
end

-- blocks in the piece
p_bl = {}
function init_p_bl()
	for i=1,4 do
		p_bl[i] = {}
		p_bl[i].x = 1
		p_bl[i].y = 1
	end
end

-- blocks in the next piece
np_bl = {}
function init_next_piece()
	for i=1,4 do
		np_bl[i] = {}
		np_bl[i].x = 0
		np_bl[i].y = 0
	end
end

-- the bag of upcoming pieces
rand_bag = {}
function init_rand_bag()
	for i=1,rand_bag_size do
		rand_bag[i] = "lin"
	end
end

--[[ blocks in the placing
animation ]]
pa_bl = {}
function init_place_anim()
	for i=1,4 do
		pa_bl[i] = {}
		pa_bl[i].x = 1
		pa_bl[i].y = 1
	end
end

-- global functions ------------

function play_sfx(i,ch)
	if(sfx_on) then sfx(i,ch) end
end

music_playing = false
function play_music()
	if(music_on) then
		if(not music_playing) then
			music(0)
			music_playing = true
		end
	else
		music_playing = false
	end
end
function stop_music()
	music_playing = false
	music(-1)
end

function load_save_data()
	if(not
	   cartdata("4block_save_1"))
	then
		sfx_on = true
		music_on = true
		start_level = 0
		game_type = 0
		ln_height = 0
		style = 1
		background = true
		highscore_a = 0
		highscore_b = 0
		highscore_c = -1
		save_save_data()
	end
	
	sfx_on = (dget(0) == 1)
	music_on = (dget(1) == 1)
	start_level = dget(2)
	game_type = dget(3)
	ln_height = dget(4)
	style = dget(5)
	background = (dget(6) == 1)
	highscore_a = dget(7)
	highscore_b = dget(8)
	highscore_c = dget(9)
end

function save_save_data()
	if(sfx_on) then dset(0,1)
	else dset (0,0) end
	if(music_on) then dset(1,1)
	else dset (1,0) end
	dset(2,start_level)
	dset(3,game_type)
	dset(4,ln_height)
	dset(5,style)
	if(background) then dset(6,1)
	else dset(6,0) end
	dset(7,highscore_a)
	dset(8,highscore_b)
	dset(9,highscore_c)
end

function init_menu_items()
	local sfx_t = "sfx: off"
	if(sfx_on) then
		sfx_t = "sfx: on"
	end
	menuitem(1,sfx_t,
	  function() toggle_sfx() end)
	
	local mus_t = "music: off"
	if(music_on) then
		mus_t = "music: on"
	end
	menuitem(2,mus_t,
	  function() toggle_music() end)
end

function set_highscore_a(sc)
	highscore_a = sc
	hsa_s =
	  "highscore a: "..highscore_a
	if(highscore_a > 0) then
		hsa_s = hsa_s.."0"
	end
	hsa_s_pos = 64 - (#hsa_s * 2)
end
function set_highscore_b(sc)
	highscore_b = sc
	hsb_s =
	  "highscore b: "..highscore_b
	if(highscore_b > 0) then
		hsb_s = hsb_s.."0"
	end
	hsb_s_pos = 64 - (#hsb_s * 2)
end
function set_highscore_c(sc)
	highscore_c = sc
	if(highscore_c <= 0) then
		hsc_s = "highscore c: na"
	else
		hsc_s =
		  "highscore c: "..highscore_c
	end
	hsc_s_pos = 64 - (#hsc_s * 2)
end

back_col = 0
function set_rand_back_col()
	local new_col = back_col
	while(new_col == back_col) do
		new_col = flr(rnd(5))+1
	end
	back_col = new_col
end

-- callback functions ----------

function reset_hs()
	set_highscore_a(0)
	set_highscore_b(0)
	set_highscore_c(-1)
end

function toggle_sfx()
	if(sfx_on) then
		sfx_on = false
		menuitem(1,"sfx: off",
			function() toggle_sfx() end)
	else
		sfx_on = true
		menuitem(1,"sfx: on",
			function() toggle_sfx() end)
	end
	save_save_data()
end

function toggle_music()
	if(music_on) then
		music_on = false
		stop_music()
		menuitem(2,"music: off",
			function() toggle_music()
			end)
	else
		music_on = true
		if(global_state == "gm" and
		   game_state ~= "go" and
		   game_state ~= "ct") then
			play_music()
		end
		menuitem(2,"music: on",
			function() toggle_music()
				end)
	end
	save_save_data()
end

-- start screen ----------------

function start_start_screen()
	global_state = "ss"
	ss_anim_ct = 0
end

function draw_start_screen()
	ss_anim_ct += 1
	local fill_col = 7
	if(ss_anim_ct == 2) then
		play_sfx(5,0)
	end
	if(ss_anim_ct < 5) then
		fill_col = 0
	elseif(ss_anim_ct < 10) then
		fill_col = 5
	elseif(ss_anim_ct < 15) then
		fill_col = 6
	elseif(ss_anim_ct == 40) then
		start_main_menu()
	end
	
	rectfill(0,0,127,127,fill_col)
	sspr(0,64,32,32,48,48)
	print("apothem",50,80,5)
end

-- main menu start -------------

function start_main_menu()
	global_state = "mm"
	set_rand_back_col()
	stop_music()
	set_mm_menu()
	sel = 1
end

function set_mm_menu()
	menuitem(3,"reset highscores",
		function() reset_hs() end)
end

-- main menu loop --------------

function update_main_menu()
	if(btnp(2)) then
		play_sfx(0,0)
		sel -= 1
		if(sel == 0) then sel = 6 end
	end
	if(btnp(3)) then
		play_sfx(0,0)
		sel += 1
		if(sel == 7) then sel = 1 end
	end
	
	if(btnp(0)) then
		play_sfx(0,0)
		if(sel==1) then dec_lv()
		elseif(sel==2) then dec_typ()
		elseif(sel==3) then dec_lh()
		elseif(sel==4) then dec_sty()
		elseif(sel==5) then tog_bg()
		end
	end
	if(btnp(1)) then
		play_sfx(0,0)
		if(sel==1) then inc_lv()
		elseif(sel==2) then inc_typ()
		elseif(sel==3) then inc_lh()
		elseif(sel==4) then inc_sty()
		elseif(sel==5) then tog_bg()
		end
	end
	
	if(sel == 6) then
		if(btnp(4) or btnp(5)) then
			play_sfx(0,0)
			start_game()
		end
	end
end

function inc_lv()
	start_level += 1
	if(start_level == 21) then
		start_level = 0
	end
end
function dec_lv()
	start_level -= 1
	if(start_level == -1) then
		start_level = 20
	end
end

function inc_typ()
	game_type += 1
	if(game_type == 3) then
		game_type = 0
	end
end
function dec_typ()
	game_type -= 1
	if(game_type == -1) then
		game_type = 2
	end
end

function inc_lh()
	ln_height += 1
	if(ln_height == 18) then
		ln_height = 0
	end
end
function dec_lh()
	ln_height -= 1
	if(ln_height == -1) then
		ln_height = 17
	end
end

function inc_sty()
	style += 1
	if(style == 5) then
		style = 1
	end
end
function dec_sty()
	style -= 1
	if(style == 0) then
		style = 4
	end
end

function tog_bg()
	background = not background
end

-- game start ------------------

function start_game()
	save_save_data()
	global_state = "gm"
	game_state = "fl"
	score = 0
	lines = 0
	cl_seq = 0
	lin_sep = 0
	sz_seq = 0
	do_draw_game_over = false
	do_draw_blocks = true
	win = false
	bottom_cleared = false
	set_level(start_level, true)
	update_score_display()
	play_music()
	set_gm_menu()
	
	reset_grid()
	if(game_type == 2) then
		rand_grid(c_line_height)
	elseif(ln_height > 0) then
		rand_grid(ln_height)
	end
	
	gen_rand_bag()
	next_piece = rand_bag[1]
	next_bag_pos = 1
	spawn_next_piece()
end

function set_gm_menu()
	menuitem(3,"quit to menu",
		function() start_main_menu()
		end)
end

function reset_grid()
	for ix=1,10 do
		for iy=1,20 do
			grid[ix][iy].bl = false
		end
	end
end

function rand_grid(lh)
	for iy=20,21-lh,-1 do
		local num_b_bl =
		  flr(rnd(max_b_bl
		  - min_b_bl+1)) + min_b_bl
		local bl_to_choose = {}
		for ix=1,10 do
			add(bl_to_choose, ix)
		end
		local num_bl_to_ch = 10
		while(num_b_bl > 0) do
			local rnd_bl =
			  flr(rnd(num_bl_to_ch))+1
			local bl_x =
			  bl_to_choose[rnd_bl]
			del(bl_to_choose, bl_x)
			num_bl_to_ch -= 1
			num_b_bl -= 1
			
			grid[bl_x][iy].bl = true
			local rnd_sp = flr(rnd(7))
			if(style == 1) then
				rnd_sp += 4
			elseif(style == 2) then
				rnd_sp += 20
			elseif(style == 3) then
				rnd_sp += 36
			else
				rnd_sp += 52
			end
			grid[bl_x][iy].sp = rnd_sp
		end
	end
end

-- game functions --------------

function set_level(lv,is_start)
	level = lv
	if(lv > 20) then
		gravity = lv_gravity[21]
	else
		gravity = lv_gravity[lv+1]
	end
	
	if(is_start) then
		lin_to_next_lv =
		  (lv * start_lv_mul)
		  + lines_to_next_lv
	else
		lin_to_next_lv =
		  lin_to_next_lv
		  + lines_to_next_lv
	end
	
	update_level_display()
	set_rand_back_col()
end

function spawn_next_piece()
	reset_controls()
	game_state = "fl"
	p_active = true
	
	local this_piece = next_piece
	
	next_bag_pos += 1
	if(next_bag_pos>rand_bag_size)
	then
		gen_rand_bag()
		next_bag_pos = 1
	end
	next_piece = rand_bag[
	  next_bag_pos]
	update_next_display()
	
	if(this_piece == "lin") then
		set_piece(4,-1,0,this_piece)
	else
		set_piece(4,0,0,this_piece)
	end
	
	if(check_collision()) then
		set_game_over()
	end
end

function reset_controls()
	fall_ct = 0
	move_delay_ct = 0
	move_ct = 0
	down_fall_ct = 0
	moving = false
	ccw_pressed = false
	cc_pressed = false
	left_pressed = false
	right_pressed = false
	-- -1=none, 0=left, 1=right
	last_pressed = -1
	rel_ccw_first = false
	rel_cc_first = false
	rel_left_first = false
	rel_right_first = false
	rel_down_first = false
	rel_up_first = false
	if(btn(4)) then
		rel_ccw_first = true
	end
	if(btn(5)) then
		rel_cc_first = true
	end
	if(btn(0)) then
		rel_left_first = true
	end
	if(btn(1)) then
		rel_right_first = true
	end
	if(btn(3)) then
		rel_down_first = true
	end
	if(btn(2)) then
		rel_up_first = true
	end
end

function check_collision()
	for i=1,4 do
		local x = p_bl[i].x
		local y = p_bl[i].y
		
		if(x < 1) then
			return true
		elseif(x > 10) then
			return true
		elseif(y > 20) then
			return true
		end
		if(y > 0) then
			if(grid[x][y].bl)
			then
				return true
			end
		end
	end
	return false
end

function set_piece(x,y,rot,typ)
	p_bbx = x p_bby = y
	p_rot = rot
	p_type = typ
	
	if(typ == "lin") then
		if(style == 1) then
			p_spr = 4
		elseif(style == 2) then
			p_spr = 20
		elseif(style == 3) then
			p_spr = 36
		else
			p_spr = 52
		end
		if(rot==0 or rot==180) then
			p_bl[1].x = x
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+2
			p_bl[3].x = x+2
			p_bl[3].y = y+2
			p_bl[4].x = x+3
			p_bl[4].y = y+2
		else
			p_bl[1].x = x+1
			p_bl[1].y = y+3
			p_bl[2].x = x+1
			p_bl[2].y = y+2
			p_bl[3].x = x+1
			p_bl[3].y = y+1
			p_bl[4].x = x+1
			p_bl[4].y = y
		end
	elseif(typ == "l_r") then
		if(style == 1) then
			p_spr = 5
		elseif(style == 2) then
			p_spr = 21
		elseif(style == 3) then
			p_spr = 37
		else
			p_spr = 53
		end
		if(rot == 0) then
			p_bl[1].x = x
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+2
			p_bl[3].y = y+1
			p_bl[4].x = x+2
			p_bl[4].y = y+2
		elseif(rot == 90) then
			p_bl[1].x = x+1
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y
			p_bl[4].x = x+2
			p_bl[4].y = y
		elseif(rot == 180) then
			p_bl[1].x = x+2
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x
			p_bl[3].y = y+1
			p_bl[4].x = x
			p_bl[4].y = y
		else
			p_bl[1].x = x+1
			p_bl[1].y = y
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y+2
			p_bl[4].x = x
			p_bl[4].y = y+2
		end
	elseif(typ == "l_l") then
		if(style == 1) then
			p_spr = 6
		elseif(style == 2) then
			p_spr = 22
		elseif(style == 3) then
			p_spr = 38
		else
			p_spr = 54
		end
		if(rot == 0) then
			p_bl[1].x = x+2
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x
			p_bl[3].y = y+1
			p_bl[4].x = x
			p_bl[4].y = y+2
		elseif(rot == 90) then
			p_bl[1].x = x+1
			p_bl[1].y = y
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y+2
			p_bl[4].x = x+2
			p_bl[4].y = y+2
		elseif(rot == 180) then
			p_bl[1].x = x
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+2
			p_bl[3].y = y+1
			p_bl[4].x = x+2
			p_bl[4].y = y
		else
			p_bl[1].x = x+1
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y
			p_bl[4].x = x
			p_bl[4].y = y
		end
	elseif(typ == "t") then
		if(style == 1) then
			p_spr = 7
		elseif(style == 2) then
			p_spr = 23
		elseif(style == 3) then
			p_spr = 39
		else
			p_spr = 55
		end
		if(rot == 0) then
			p_bl[1].x = x
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+2
			p_bl[3].y = y+1
			p_bl[4].x = x+1
			p_bl[4].y = y+2
		elseif(rot == 90) then
			p_bl[1].x = x+1
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y
			p_bl[4].x = x+2
			p_bl[4].y = y+1
		elseif(rot == 180) then
			p_bl[1].x = x+2
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x
			p_bl[3].y = y+1
			p_bl[4].x = x+1
			p_bl[4].y = y
		else
			p_bl[1].x = x+1
			p_bl[1].y = y
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y+2
			p_bl[4].x = x
			p_bl[4].y = y+1
		end
	elseif(typ == "s") then
		if(style == 1) then
			p_spr = 8
		elseif(style == 2) then
			p_spr = 24
		elseif(style == 3) then
			p_spr = 40
		else
			p_spr = 56
		end
		if(rot==0 or rot==180) then
			p_bl[1].x = x
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+2
			p_bl[3].x = x+1
			p_bl[3].y = y+1
			p_bl[4].x = x+2
			p_bl[4].y = y+1
		else
			p_bl[1].x = x+1
			p_bl[1].y = y+2
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x
			p_bl[3].y = y+1
			p_bl[4].x = x
			p_bl[4].y = y
		end
	elseif(typ == "z") then
		if(style == 1) then
			p_spr = 9
		elseif(style == 2) then
			p_spr = 25
		elseif(style == 3) then
			p_spr = 41
		else
			p_spr = 57
		end
		if(rot==0 or rot==180) then
			p_bl[1].x = x
			p_bl[1].y = y+1
			p_bl[2].x = x+1
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y+2
			p_bl[4].x = x+2
			p_bl[4].y = y+2
		else
			p_bl[1].x = x
			p_bl[1].y = y+2
			p_bl[2].x = x
			p_bl[2].y = y+1
			p_bl[3].x = x+1
			p_bl[3].y = y+1
			p_bl[4].x = x+1
			p_bl[4].y = y
		end
	else -- type is "sqr"
		if(style == 1) then
			p_spr = 10
		elseif(style == 2) then
			p_spr = 26
		elseif(style == 3) then
			p_spr = 42
		else
			p_spr = 58
		end
		p_bl[1].x = x+1
		p_bl[1].y = y+1
		p_bl[2].x = x+2
		p_bl[2].y = y+1
		p_bl[3].x = x+1
		p_bl[3].y = y+2
		p_bl[4].x = x+2
		p_bl[4].y = y+2 
	end
end

function gen_rand_bag()
	local num_lin, num_l_r = 0, 0
	local num_l_l, num_t = 0, 0
	local num_s, num_z = 0, 0
	local num_sqr = 0
	local num_choose = 0
	local p_to_choose = {}
	
	for i=1,rand_bag_size do
		local include_lin = true
		local include_l_r = true
		local include_l_l = true
		local include_t = true
		local include_s = true
		local include_z = true
		local include_sqr = true
		
		if(lin_sep >= max_lin_sep)
		then
			rand_bag[i] = "lin"
			lin_sep = 0
			num_lin += 1
			sz_seq = 0
		else
			if(sz_seq >= max_sz_seq)
			then
				include_s = false
				include_z = false
			end
			
			if(num_lin>=max_appear) then
				include_lin = false
			end
			if(num_l_r>=max_appear) then
				include_l_r = false
			end
			if(num_l_l>=max_appear) then
				include_l_l = false
			end
			if(num_t>=max_appear) then
				include_t = false
			end
			if(num_s>=max_appear) then
				include_s = false
			end
			if(num_z>=max_appear) then
				include_z = false 
			end
			if(num_sqr>=max_appear) then
				include_sqr = false
			end
			
			num_choose = 0
			if(include_lin) then
				num_choose += 1
				p_to_choose[
				num_choose] = "lin"
			end
			if(include_l_r) then
				num_choose += 1
				p_to_choose[
				num_choose] = "l_r"
			end
			if(include_l_l) then
				num_choose += 1
				p_to_choose[
				num_choose] = "l_l"
			end
			if(include_t) then
				num_choose += 1
				p_to_choose[
				num_choose] = "t"
			end
			if(include_s) then
				num_choose += 1
				p_to_choose[
				num_choose] = "s"
			end
			if(include_z) then
				num_choose += 1
				p_to_choose[
				num_choose] = "z"
			end
			if(include_sqr) then
				num_choose += 1
				p_to_choose[
				num_choose] = "sqr"
			end
			
			if(num_choose == 0) then
					num_choose = 5
					p_to_choose[1] = "lin"
					p_to_choose[2] = "l_r"
					p_to_choose[3] = "l_l"
					p_to_choose[4] = "t"
					p_to_choose[5] = "sqr"
				if(sz_seq<max_sz_seq) then
					num_choose = 7
					p_to_choose[6] = "s"
					p_to_choose[7] = "z"
				end
			end
			
			local r =
			  flr(rnd(num_choose))+1
			local r_piece =
			  p_to_choose[r]
			
			if(r_piece == "lin") then
				num_lin += 1
				lin_sep = 0
			else
				lin_sep += 1
			end
			
			if(r_piece == "s") then
				num_s += 1
				sz_seq += 1
			elseif(r_piece == "z") then
				num_z += 1
				sz_seq += 1
			else
				sz_seq = 0
			end
			
			if(r_piece == "l_r") then
				num_l_r += 1
			elseif(r_piece == "l_l") then
				num_l_l += 1
			elseif(r_piece == "t") then
				num_t += 1
			elseif(r_piece == "sqr") then
				num_sqr += 1
			end
			
			rand_bag[i] = r_piece
		end
	end
end

function set_game_over()
	if(game_type == 0) then
		if(score > highscore_a) then
			set_highscore_a(score)
			save_save_data()
		end
	elseif(game_type==1) then
		if(score > highscore_b) then
			set_highscore_b(score)
			save_save_data()
		end
	elseif(win) then
		if(score < highscore_c or
		   highscore_c == -1) then
			set_highscore_c(score)
			save_save_data()
		end
	end
	
	game_state = "go"
	do_draw_game_over = true
	go_anim_ct = 0
	go_anim_pos = 0
	stop_music()
	
	if(win) then
		play_sfx(7,1)
	else
		play_sfx(6,1)
	end
end

-- game loop -------------------

function update_game()
	if(game_state == "fl") then
		update_game_falling()
	elseif(game_state == "cl") then
		update_game_clearing()
	elseif(game_state == "go") then
		update_game_over()
	else
		update_game_continue()
	end
end

-- game falling loop -----------

function update_game_falling()
	--[[ rotate counter-clockwise
		 button ]]
	if(btn(4)) then
		if(not ccw_pressed and
		   not rel_ccw_first) then
			ccw_pressed = true
			rotate_piece_ccw()
		end
	else
		rel_ccw_first = false
		ccw_pressed = false
	end
	
	-- rotate clockwise button
	if(btn(5)) then
		if(not cw_pressed and
		   not rel_first) then
			cw_pressed = true
			rotate_piece_cw()
		end
	else
		rel_cc_first = false
		cw_pressed = false
	end
	
	-- move left button
	if(btn(0)) then
		if(not left_pressed and
		   not rel_left_first) then
			left_pressed = true
			move_delay_ct = 0
			move_ct = 0
			moving = false
			last_pressed = 0
			move_piece_left()
		elseif(last_pressed == 0) then
			if(moving or
			   move_delay <= 0) then
				move_ct += move_speed
				if(move_ct >= 1) then
					move_ct -= 1
					move_piece_left()
				end
			else
				move_delay_ct += 1
				if(move_delay_ct >=
				   move_delay) then
					moving = true
				end
			end
		end
	else
		rel_left_first = false
		left_pressed = false
	end
	
	-- move right button
	if(btn(1)) then
		if(not right_pressed and
		   not rel_right_first) then
			right_pressed = true
			move_delay_ct = 0
			move_ct = 0.0
			moving = false
			last_pressed = 1
			move_piece_right()
		elseif(last_pressed == 1) then
			if(moving or
			   move_delay <= 0) then
				move_ct += move_speed
				if(move_ct >= 1) then
					move_ct -= 1
					move_piece_right()
				end
			else
				move_delay_ct += 1
				if(move_delay_ct >=
				   move_delay) then
					moving = true
				end
			end
		end
	else
		rel_right_first = false
		right_pressed = false
	end
	
	-- instant drop button
	up_pressed = false
	if(btn(2)) then
		if(not rel_up_first) then
			up_pressed = true
			instant_drop()
		end
	else
		rel_up_first = false
	end
	
	-- move down button
	down_down = false
	if(btn(3)) then
		if(not up_pressed and
		   down_speed > gravity)
		then
			if(not rel_down_first) then
				down_fall_ct += down_speed
				if(down_fall_ct >= 1) then
					down_fall_ct -= 1
					move_piece_down()
				end
			down_down = true
			end
		end
	else
		rel_down_first = false
		down_fall_ct = 0
	end
	
	-- gravity
	if(not down_down and
	   not up_pressed) then
		fall_ct += gravity
		if(fall_ct >= 1) then
			fall_ct -= 1
			move_piece_down()
		end
	end
end

function rotate_piece_ccw()
	local prev_rot = p_rot
	
	p_rot += 90
	if(p_rot == 360) then
		p_rot = 0
	end
	
	set_piece(p_bbx,p_bby,p_rot,
	          p_type)
	
	if(check_collision()) then
		p_rot = prev_rot
		set_piece(p_bbx,p_bby,p_rot,
		          p_type)
	else
		play_sfx(2,0)
	end
end
function rotate_piece_cw()
	local prev_rot = p_rot
	
	p_rot -= 90
	if(p_rot == -90) then
		p_rot = 270
	end
	
	set_piece(p_bbx,p_bby,p_rot,
	          p_type)
	
	if(check_collision()) then
		p_rot = prev_rot
		set_piece(p_bbx,p_bby,p_rot,
		          p_type)
	else
		play_sfx(2,0)
	end
end

function move_piece_left()
	for i=1,4 do
		p_bl[i].x -= 1
	end
	p_bbx -= 1
	
	if(check_collision()) then
		for i=1,4 do
			p_bl[i].x += 1
		end
		p_bbx += 1
	else
		play_sfx(1,1)
	end
end
function move_piece_right()
	for i=1,4 do
		p_bl[i].x += 1
	end
	p_bbx += 1
	
	if(check_collision()) then
		for i=1,4 do
			p_bl[i].x -= 1
		end
		p_bbx -= 1
	else
		play_sfx(1,1)
	end
end

function move_piece_down()
	for i=1,4 do
		p_bl[i].y += 1
	end
	p_bby += 1
	
	if(check_collision()) then
		for i=1,4 do
			p_bl[i].y -= 1
		end
		p_bby -= 1
		place_block()
	end
end

function instant_drop()
	placed = false
	while(not placed) do
		move_piece_down()
	end
end

function place_block()
	p_active = false
	placed = true
	
	play_sfx(3,1)
	start_place_anim()
	
	for i=1,4 do
		x = p_bl[i].x
		y = p_bl[i].y
		if(y > 0) then
			grid[x][y].bl = true
			grid[x][y].sp = p_spr
		end
	end
	
	if(game_type == 2) then
		score += 1
		update_score_display()
	end
	
	check_lines()
end

function start_place_anim()
	num_pa_bl = 0
	for i=1,4 do
		bl_below = false
		if(p_bl[i].y == 20) then
			bl_below = true
		elseif(p_bl[i].y < 0) then
			bl_below = false
		elseif(grid[p_bl[i].x]
		           [p_bl[i].y+1].bl)
		then
			bl_below = true
		end
		
		if(bl_below) then
			num_pa_bl += 1
			pa_bl[num_pa_bl].x=p_bl[i].x
			pa_bl[num_pa_bl].y =
			  p_bl[i].y+1
		end
	end
	
	if(num_pa_bl > 0) then
		do_place_anim = true
		pa_ct = 0
	end
end

l_to_cl = {}
function check_lines()
	num_cl = 0
	
	for iy=1,20 do
		local is_full = true
		for ix=1,10 do
			if(not grid[ix][iy].bl) then
				is_full = false
				break
			end
		end
		if(is_full) then
			if(iy == 20) then
				bottom_cleared = true
			end
			num_cl += 1
			l_to_cl[num_cl] = iy
			if(num_cl == 4) then
				break
			end
		end
	end
	
	if(num_cl == 0) then
		cl_seq = 0
		spawn_next_piece()
	else
		clear_lines()
	end
end

function clear_lines()
	game_state = "cl"
	clr_anim_ct = 0
	if(num_cl == 4) then
		play_sfx(5,1)
	else
		play_sfx(4,1)
	end
end

-- game clearing loop ----------

function update_game_clearing()
	clr_anim_ct += 1
	if(clr_anim_ct == 1) then
		clr_trans_white()
	elseif(clr_anim_ct == 4) then
		set_clr_sprite(46)
	elseif(clr_anim_ct == 5) then
		set_clr_sprite(47)
	elseif(clr_anim_ct == 6) then
		set_clr_sprite(62)
	elseif(clr_anim_ct == 7) then
		set_clr_sprite(63)
	elseif(clr_anim_ct == 8) then
		drop_blocks()
	end
end

function clr_trans_white()
	for i=1,num_cl do
		local iy = l_to_cl[i]
		for ix=1,10 do
			if(grid[ix][iy].sp<=10) then
				grid[ix][iy].sp = 12
			elseif(grid[ix][iy].sp>=52)
			then
				grid[ix][iy].sp = 60
			elseif(grid[ix][iy].sp>=36)
			then
				grid[ix][iy].sp = 44
			elseif(grid[ix][iy].sp==20
			    or grid[ix][iy].sp==23)
			then
				grid[ix][iy].sp = 28
			elseif(grid[ix][iy].sp==21)
			then
				grid[ix][iy].sp = 29
			elseif(grid[ix][iy].sp==22
			    or grid[ix][iy].sp==24)
			then
				grid[ix][iy].sp = 30
			else
				grid[ix][iy].sp = 31
			end
		end
	end
end

function set_clr_sprite(s)
	for i=1,num_cl do
		local iy = l_to_cl[i]
		for ix=1,10 do
			grid[ix][iy].sp = s
		end
	end
end

function drop_blocks()
	local bottom = l_to_cl[num_cl]
	local drop_amt = 0
	for iy=bottom,1,-1 do
		local drop_line = true
		if(drop_amt < 4) then
			if(iy==l_to_cl[num_cl
			   - drop_amt])
			then
				drop_amt += 1
				drop_line = false
			end
		end
		
		if(drop_line) then
			local dy = iy + drop_amt
			for ix=1,10 do
				grid[ix][dy].bl =
				  grid[ix][iy].bl
				grid[ix][dy].sp =
				  grid[ix][iy].sp
				grid[ix][iy].bl = false
			end
		else
			for ix=1,10 do
				grid[ix][iy].bl = false
			end
		end
	end
	
	inc_num_lines()
	
	if(game_type == 2) then
		if(bottom_cleared) then
			win = true
			set_game_over()
		else
			spawn_next_piece()
		end
	elseif(game_type == 1) then
		if(lines >= b_lines) then
			win = true
			set_game_over()
		else
			spawn_next_piece()
		end
	else
		spawn_next_piece()
	end
end

function inc_num_lines()
	if(game_type ~= 2) then
		if(num_cl == 1) then
			mul = score_1_mul
		elseif(num_cl == 2) then
			mul = score_2_mul
		elseif(num_cl == 3) then
			mul = score_3_mul
		else
			mul = score_4_mul
		end
		
		cl_seq += 1
		if(cl_seq > 1) then
			mul *= consec_lin_mul
		end
		
		pts = flr(mul * gravity)
		
		if(32767-score < pts) then
			score = 32767
		else
			score += pts
		end
	end
	
	lines += num_cl
	if(lines>=lin_to_next_lv)
	then
		set_level(level+1, false)
	end
	
	update_score_display()
end

-- game over loop --------------

function update_game_over()
	if(go_anim_pos == 123) then
		game_state = "ct"
	end
end

-- game continue loop ----------

function update_game_continue()
	if(btnp(0) or btnp(1) or
	   btnp(2) or btnp(3) or
	   btnp(4) or btnp(5)) then
		play_sfx(0,0)
		start_main_menu()
	end
end

-- background drawing ----------

back_pos = 0
function draw_background()
	rectfill(0,0,127,127,0)
	if(background) then
		pal(5,back_col)
		back_pos += back_speed
		if(back_pos >= 50) then
			back_pos -= 50
		end
		
		for ix=0,3 do
			local x = ix*50 - back_pos
			for iy=0,2 do
				spr(1,x,iy*50+back_pos)
			end
		end
		pal(5,5)
	end
end

-- main menu drawing -----------

function draw_main_menu()
	draw_background()
	draw_mm_border()
	draw_version()
	draw_logo()
	draw_highscore()
	draw_options()
	draw_selection()
end

function draw_mm_border()
	rectfill(1,1,5,126,6)
	rectfill(122,1,126,126,6)
	rectfill(6,1,121,4,6)
	rectfill(6,58,121,61,6)
	rectfill(6,123,121,126,6)
	
	if(style > 2) then
		spr(72,0,0)
	else
		spr(71,0,0)
	end
	
	spr(72,120,0,1,1,true,false)
	spr(72,120,120,1,1,true,true)
	spr(72,0,120,1,1,false,true)
	rectfill(8,0,119,0,5)
	rectfill(8,127,119,127,5)
	rectfill(0,8,0,119,5)
	rectfill(127,8,127,119,5)
end

function draw_version()
	print(version,104,8,5)
end

function draw_logo()
	sspr(0,32,48,32,40,12)
	
	if(style > 2) then
		spr(88,6,5)
	else
		spr(87,6,5)
	end
	
	spr(88,114,5,1,1,true,false)
	spr(88,114,50,1,1,true,true)
	spr(88,6,50,1,1,false,true)
	rectfill(14,5,113,5,5)
	rectfill(14,57,113,57,5)
	rectfill(6,13,6,49,5)
	rectfill(121,13,121,49,5)
end

function draw_highscore()
	if(game_type == 0) then
		print(hsa_s,hsa_s_pos,49,7)
	elseif(game_type==1) then
		print(hsb_s,hsb_s_pos,49,7)
	else
		print(hsc_s,hsc_s_pos,49,7)
	end
end

function draw_options()
	if(style > 2) then
		spr(104,5,62)
	else
		spr(103,5,62)
	end
	
	spr(104,115,62,1,1,true,false)
	spr(104,115,115,1,1,true,true)
	spr(104,5,115,1,1,false,true)
	rectfill(13,62,114,63,5)
	rectfill(13,121,114,122,5)
	rectfill(5,70,6,114,5)
	rectfill(121,70,122,114,5)
	
	local t_col = 6
	local t = ""
	
	if(sel == 1) then t_col = 7 end
	print("level:",43,69,t_col)
	if(start_level < 10) then
		t = "0"..start_level
	else
		t = start_level
	end
	print(t,74,69,t_col)
	
	if(sel == 2) then t_col = 7
	else t_col = 6 end
	print("type:",47,77,t_col)
	if(game_type == 0) then
		t = "marathon"
	elseif(game_type==1) then
		t = b_lines.." lines"
	else
		t = "clear"
	end
	print(t,74,77,t_col)
	
	if(sel == 3) then t_col = 7
	else t_col = 6 end
	print("line height:",19,85,
	      t_col)
	if(ln_height < 10) then
		t = "0"..ln_height
	else
		t = ln_height
	end
	print(t,74,85,t_col)
	
	if(sel == 4) then t_col = 7
	else t_col = 6 end
	print("style:",43,93,t_col)
	if(style == 1) then
		t = "default"
	elseif(style == 2) then
		t = "fancy"
	elseif(style == 3) then
		t = "simple"
	else
		t = "solid"
	end
	print(t,74,93,t_col)
	
	if(sel == 5) then t_col = 7
	else t_col = 6 end
	print("background:",23,101,
	      t_col)
	if(background) then
		t = "on"
	else
		t = "off"
	end
	print(t,74,101,t_col)
	
	if(sel == 6) then t_col = 7
	else t_col = 6 end
	print("start",54,111,t_col)
end

function draw_selection()
	local s1 = 74
	local s2 = 75
	if(style > 2) then
		s1 = 90 s2 = 91
	end
	if(sel == 1) then
		spr(s1,68,69) spr(s2,84,69)
	elseif(sel == 2) then
		spr(s1,68,77) spr(s2,108,77)
	elseif(sel == 3) then
		spr(s1,68,85) spr(s2,84,85)
	elseif(sel == 4) then
		spr(s1,68,93) spr(s2,104,93)
	elseif(sel == 5) then
		spr(s1,68,101) spr(s2,88,101)
	else
		spr(s1,48,111) spr(s2,76,111)
	end
end

-- game drawing ----------------

function draw_game()
	draw_background()
	draw_game_border()
	draw_grid()
	draw_piece()
	draw_gm_border_front()
	draw_place_anim()
	draw_score_display()
	draw_next_display()
	draw_level_display()
	draw_game_over()
end

function draw_game_border()
	spr(32,0,120)
	spr(33,60,120)
	spr(18,120,0)
	spr(34,120,120)
	
	rectfill(0,8,0,119,5)
	rectfill(1,8,3,119,6)
	rectfill(4,8,5,119,5)
	
	rectfill(8,127,59,127,5)
	rectfill(8,126,59,126,6)
	rectfill(8,124,59,125,5)
	
	rectfill(66,8,67,119,5)
	rectfill(68,4,69,123,6)
	
	rectfill(127,8,127,119,5)
	rectfill(125,8,126,119,6)
	
	rectfill(68,0,119,0,5)
	rectfill(68,1,119,3,6)
	
	rectfill(68,127,119,127,5)
	rectfill(68,124,119,126,6)
	
	rectfill(70,49,124,51,6)
	rectfill(100,52,124,75,6)
	rectfill(70,76,124,78,6)
	rectfill(70,105,124,123,6)
end

function draw_gm_border_front()
	if(style > 2) then
		spr(16,0,0)
	else
		spr(0,0,0)
	end
	spr(17,60,0)
	rectfill(8,0,59,0,5)
	rectfill(8,1,59,1,6)
	rectfill(8,2,59,3,5)
end

function draw_grid()
	if(do_draw_blocks) then
		for ix=1,10 do
			for iy=1,20 do
				if(grid[ix][iy].bl) then
					spr(grid[ix][iy].sp,
					  ix*6,iy*6 - 2)
				end
			end
		end
	end
end

function draw_piece()
	if(p_active and do_draw_blocks)
	then
		for i=1,4 do
			spr(p_spr,
			  p_bl[i].x*6,p_bl[i].y*6-2)
		end
	end
end

function update_next_display()
	set_piece(12,8,0,next_piece)
	
	for i=1,4 do
		np_bl[i].x = p_bl[i].x
		np_bl[i].y = p_bl[i].y
	end
	np_spr = p_spr
	np_type = p_type
	for i=1,4 do
		np_bl[i].x *= 6
		np_bl[i].x += 1
		np_bl[i].y *= 6
		np_bl[i].y += 4
	end
	if(np_type == "lin") then
		for i=1,4 do
			np_bl[i].y -= 3
		end
	elseif(np_type ~= "sqr") then
		for i=1,4 do
			np_bl[i].x += 3
		end
	end
	
	do_draw_nxt_d = true
end
function draw_next_display()
	if(style > 2) then
		spr(49,70,52)
	else
		spr(48,70,52)
	end
	
	spr(49,92,52,1,1,true,false)
	spr(49,92,68,1,1,true,true)
	spr(49,70,68,1,1,false,true)
	rectfill(78,52,91,52,5)
	rectfill(78,75,91,75,5)
	rectfill(70,60,70,67,5)
	rectfill(99,60,99,67,5)
	
	for i=1,4 do
		spr(np_spr,
		  np_bl[i].x,np_bl[i].y)
	end
end

function update_score_display()
	if(score == 0 or
	   game_type == 2) then
		sc_s = ""..score
	else
		sc_s = ""..score.."0"
	end
	sc_s_pos = 98 - (#sc_s * 2)
	
	ln_s = ""..lines
	ln_s_pos = 98 - (#ln_s * 2)
end
function draw_score_display()
	if(style > 2) then
		spr(49,70,4)
	else 
		spr(48,70,4)
	end
	
	spr(49,117,4,1,1,true,false)
	spr(49,117,41,1,1,true,true)
	spr(49,70,41,1,1,false,true)
	rectfill(78,4,116,4,5)
	rectfill(78,48,116,48,5)
	rectfill(70,12,70,40,5)
	rectfill(124,12,124,40,5)
	
	print("score",88,10,6)
	print(sc_s,sc_s_pos,19,7)
	
	print("lines",88,29,6)
	print(ln_s,ln_s_pos,38,7)
end

function update_level_display()
	lv_s = ""..level
	lv_s_pos = 98 - (#lv_s * 2)
end
function draw_level_display()
	if(style > 2) then
		spr(49,70,79)
	else
		spr(48,70,79)
	end
	
	spr(49,117,79,1,1,true,false)
	spr(49,117,97,1,1,true,true)
	spr(49,70,97,1,1,false,true)
	rectfill(78,79,116,79,5)
	rectfill(78,104,116,104,5)
	rectfill(70,87,70,96,5)
	rectfill(124,87,124,96,5)
	
	print("level",88,85,6)
	print(lv_s,lv_s_pos,94,7)
		
	do_draw_lv_d = false
end

function draw_place_anim()
	if(do_place_anim) then
		pa_ct += 1
		if(pa_ct == 1) then
			sp = 45
		elseif(pa_ct == 2) then
			sp = 61
		elseif(pa_ct == 3) then
			do_place_anim = false end
		
		if(do_place_anim) then
			for i=1,num_pa_bl do
				spr(sp,pa_bl[i].x*6,
			      pa_bl[i].y*6 - 2)
			end
		end
	end
end
 
function draw_game_over()
	if(do_draw_game_over) then
		if(go_anim_pos < 123) then
			go_anim_ct += 1
			go_anim_pos = go_anim_ct *
		 	  go_anim_speed + 4
			if(go_anim_pos >= 123) then
				go_anim_pos = 123
				do_draw_blocks = false
			end
			
			rectfill(6,4,65,
			         go_anim_pos,6)
			if(go_anim_pos < 123) then
				rectfill(6,go_anim_pos,65,
		           go_anim_pos+1,5)
			end
		else
			if(win) then
				print("you win!",21,50,7)
			else
				print("game over!",17,50,7)
			end
			print("press any",18,61,7)
			print("button to",18,67,7)
			print("continue",20,73,7)
		end
	end
end
__gfx__
07555555000000000000000000000000999999008888880033333300444444005555550011111100222222000000000066666600000000000000000000000000
77666666000000000000000000000000977aa900877ee800377bb300477ff40057766500177cc100277ee2000000000067777600000000000000000000000000
7666675500000000000000000000000097aaa90087eee80037bbb30047fff4005766650017ccc10027eee2000000000067777600000000000000000000000000
766675550000500000000000000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600000000000000000000000000
566675000005000000000000000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600000000000000000000000000
56665500000000000000000000000000999999008888880033333300444444005555550011111100222222000000000066666600000000000000000000000000
56665500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
56665500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0555555555555555555555500000000077a7990077e77e007b33b30077f744007655650077717c0077727e000000000077776600777777007766760077767700
55666666666666666666665500000000777aa900787e8e00b37b3b00777ff4006576560077ccc10077eee2000000000077777600767767007677670077777600
56666555555555566666666500000000a7aaaa00e7eeee0037bbb300f7ffff00576665007cccc1007eeee2000000000077777700777777006777760077777600
566655555555555566666665000000007aa7aa007eeeee003bbbb3007ff7ff00566665001cccc1002eeee2000000000077777700777777006777760067777600
566655000000005500000665000000009aaaa90078ee8e00b3bb3b004ffff400656656007cccc1007eeee2000000000067777600767767007677670077777600
5666550000000055000006650000000099aa9900eeeeee003b33b30044ff440056556500c1111c00e2222e000000000066776600777777006766760076666700
56665500000000550000066500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
56665500000000550000066500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
56665500000000550000066500000000999999008888880033333300444444005555550011111100222222000000000066666600000000007777770077077700
566655000000005500000665000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600777777007777770077077700
566655000000005500000665000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600777777007777770070777700
566655000000005500000665000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600000000007777770077770700
566655555555555666666665000000009aaaa9008eeee8003bbbb3004ffff400566665001cccc1002eeee2000000000067777600000000007777770000777000
56666555555555566666666500000000999999008888880033333300444444005555550011111100222222000000000066666600000000007777770077077700
55666666666666666666665500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
05555555555555555555555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
67555555655555550000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700000000000707000000000000
75000000550000000000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700000000007007700000070000
70000000500000000000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700000000007007770070000700
50000000500000000000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700777777000077070000000000
50000000500000000000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700777777000007700000000000
50000000500000000000000000000000aaaaaa0088888800bbbbbb00ffffff0066666600cccccc00eeeeee000000000077777700000000000700700000007000
50000000500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
50000000500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000005555555555555555555555555555555500000000075555550555555500000000007000007000000000000000777777007707770000000000
00000000000000005576666666666655557666666666665500000000770000005500000000000000079000007900000077777700777777007707770000000000
000999900999900057688888886666655763336666666665000000007000000050000000000000009a9000009a90000077777700777777007077770000000000
00097a9009aa9000576877eeee86666557637b366666666500000000700000005000000000000000099000009900000000000000777777007777070000000000
00097a9009aa900056687eeeeee8666556637b366666666500000000500000005000000000000000009000009000000000000000777777000077700000000000
00097a9009aa900056687ee8eee8666556637b366666666500000000500000005000000000000000000000000000000000000000777777007707770000000000
0009aa9999aa90005668eee8eee866655663bb366666666500000000500000005000000000000000000000000000000000000000000000000000000000000000
0009aaaaaaaa90005668eeeeee8666655663bb366666666500000000500000005000000000000000000000000000000000000000000000000000000000000000
0009aaaaaaaa90005668eeeeeee866655663bb366666666500000000675555556555555500000000009000009000000000000000070700000000000000000000
0000999999aa90005668eee88eee86655663bb366666666500000000750000005500000000000000099000009900000000000000700770000007000000000000
0000000009aa90005668eee88eee86655663bb3333336665000000007000000050000000000000009a9000009a90000000000000700777007000070000000000
0000000009aa90005668eeeeeeee86655663bbbbbbbb366500000000500000005000000000000000099000009900000077777700007707000000000000000000
0000000009aa90005668eeeeeee866655663bbbbbbbb366500000000500000005000000000000000009000009000000077777700000770000000000000000000
00000000099990005668888888866665566333333333366500000000500000005000000000000000000000000000000000000000070070000000700000000000
00000000000000005566666666666655556666666666665500000000500000005000000000000000000000000000000000000000000000000000000000000000
00000000000000005555555555555555555555555555555500000000500000005000000000000000000000000000000000000000000000000000000000000000
55555555555555555555555555555555000000000000000000000000675555556555555500000000000000000000000000000000000000000000000000000000
55766666666666555576666666666655000000000000000000000000755555555555555500000000000000000000000000000000000000000000000000000000
57666444444666655766611111116665000222000022200000000000750000005500000000000000000000000000000000000000000000000000000000000000
576647fffff46665576617cccccc166500027e2002ee200000000000550000005500000000000000000000000000000000000000000000000000000000000000
56647fffffff466556617ccccccc166500027e202eee200000000000550000005500000000000000000000000000000000000000000000000000000000000000
56647ff44fff466556617cc11111666500027e22eee2000000000000550000005500000000000000000000000000000000000000000000000000000000000000
56647f4664ff466556617c16666666650002ee2eee20000000000000550000005500000000000000000000000000000000000000000000000000000000000000
5664ff4664ff46655661cc16666666650002eeeee200000000000000550000005500000000000000000000000000000000000000000000000000000000000000
5664ff4664ff46655661cc16666666650002eeeee200000000000000000000000000000000000000000000000000000000000000000000000000000000000000
5664ff4664ff46655661cc16666666650002eeeeee20000000000000000000000000000000000000000000000000000000000000000000000000000000000000
5664fff44fff46655661ccc1111166650002ee2eeee2000000000000000000000000000000000000000000000000000000000000000000000000000000000000
5664ffffffff46655661cccccccc16650002ee22eeee200000000000000000000000000000000000000000000000000000000000000000000000000000000000
56664ffffff4666556661ccccccc16650002ee202eee200000000000000000000000000000000000000000000000000000000000000000000000000000000000
56666444444666655666611111116665000222200222200000000000000000000000000000000000000000000000000000000000000000000000000000000000
55666666666666555566666666666655000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
55555555555555555555555555555555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000050000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000005555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000055555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000055555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000055055550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000055005555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000555005555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000550005555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000550000555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000550000555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000005500000055555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000005500000005555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000055000050005555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000055000550000555555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000550000550000055555500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000550055555555555555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000005555555555555555555555000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000005555555550000055555550000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000055550005500000000055500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000555000005000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00005500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00050000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__label__
07555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555550
77666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666655
76666755555555555555555555555555555555555555555555555555555555555556666666666666666666666666666666666666666666666666666666666665
76667555555555555555555555555555555555555555555555555555555555555555666666666666666666666666666666666666666666666666666666666665
56667500000000000000000000000000000000000000000000000000000000000055666755555555555555555555555555555555555555555555555555556665
56665500000000000000000000000000000000000000000000000000000000000055667500000000000000000000000000000000000000000000000000055665
56665500000000000000000000000000000000000000000000000000000000000055667000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000660066006606660666000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006000600060606060600000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006660600060606600660000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000060600060606060600000000000000000005665
56665500000000000000000000000000000000000005000000000000000000000055665000000000000000006600066066006060666000000000000000005665
56665500000000000000000000000000000000000050000000000000000000000055665000000000000000000000500000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000077700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000077700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006000666066006660066000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006000060060606000600000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006000060060606600666000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006000060060606000006000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000006660666060606660660000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000077700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000070700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000077700000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000000000000000000000000000000000055665500000000000000000000000000000000000000000000000000055665
56665500000000000000000000000000000000000000000000000000000000000055666555555555555555555555555555555555555555555555555555556665
56665500000000000000000000000000000000000000000000000000000000000055666666666666666666666666666666666666666666666666666666666665
56665500000000000000000000000000000000000000000000000000000000000055666666666666666666666666666666666666666666666666666666666665
56665500000000000000000000000000000000000000000000000000000000000055666666666666666666666666666666666666666666666666666666666665
56665500000000000000000033333333333333333300000000000000000000000055666755555555555555555555555555566666666666666666666666666665
566655000000000000000000377bb3377bb3377bb300000000000000000000000055667500000000000000000000000000556666666666666666666666666665
56665500000000000000000037bbb337bbb337bbb300000000000000000000000055667000000000000000000000000000056666666666666666666666666665
5666550000000000000000003bbbb33bbbb33bbbb300000000000000000000000055665000000000000000000000000000056666666666666666666666666665
5666550000000000000000003bbbb33bbbb33bbbb300000000000000000000000055665000000000000000000000000000056666666666666666666666666665
56665500000000000000000033333333333333333300000000000000000000000055665000000000000000000000000000056666666666666666666666666665
56665500000000000000000033333300000000000000000000000000000000000055665000000000000000000000000000056666666666666666666666666665
566655000000000000000000377bb300000000000000000000000000000000000055665000000000000000000000000000056666666666666666666666666665
56665500000000000000000037bbb300000000000000000000000000000000000055665000000000000000000000000000056666666666666666666666666665
5666550000000000000000003bbbb300000000000000000000000000000000000055665009999999999999999999999990056666666666666666666666666665
5666550000000000000000003bbbb30000000000000000000000000000000000005566500977aa9977aa9977aa9977aa90056666666666666666666666666665
566655000000000000000000333333000000000000000000000000000000000000556650097aaa997aaa997aaa997aaa90056666666666666666666666666665
56665500000000000000000000000000000000000005000000000022222222222255665009aaaa99aaaa99aaaa99aaaa90056666666666666666666666666665
566655000000000000000000000000000000000000500000000000277ee2277ee255665009aaaa99aaaa99aaaa99aaaa90056666666666666666666666666665
56665500000000000000000000000000000000000000000000000027eee227eee255665009999999999999999999999990056666666666666666666666666665
5666550000000000000000000000000000000000000000000000002eeee22eeee255665000000000000000000000000000056666666666666666666666666665
5666550000000000000000000000000000000000000000000000002eeee22eeee255665000000000000000000000000000056666666666666666666666666665
56665500000000000000000000000000000000000000000000000022222222222255665000000000000000000000000000056666666666666666666666666665
56665500000000000000000000000000000000000000000000000022222222222255665000000000000000000000000000056666666666666666666666666665
566655000000000000000000000000000000000000000000000000277ee2277ee255665000000000000000000000000000056666666666666666666666666665
56665500000000000000000000000000000000000000000000000027eee227eee255665000000000000000000000000000056666666666666666666666666665
5666550000000000000000000000000000000000000000000000002eeee22eeee255665000000000000000000000000000056666666666666666666666666665
5666550000000000000000000000000000000000000000000000002eeee22eeee255665500000000000000000000000000556666666666666666666666666665
56665500000000000000000000000000000000000000000000000022222222222255666555555555555555555555555555566666666666666666666666666665
56665500000000000000000000000000000000000000000000000022222222222255666666666666666666666666666666666666666666666666666666666665
566655000000000000000000000000000000000000000000000000277ee2277ee255666666666666666666666666666666666666666666666666666666666665
56665500000000000000000000000000000000000000000000000027eee227eee255666666666666666666666666666666666666666666666666666666666665
5666550000000000000000000000000000000000000000000000002eeee22eeee255666755555555555555555555555555555555555555555555555555556665
5666550000000000000000000000000000000000000000000000002eeee22eeee255667500000000000000000000000000000000000000000000000000055665
56665500000000000000000000000000000000000000000000000022222222222255667000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000055555555555500000022222222222255665000000000000000000000000000000000000000000000000000005665
566655000000000000000000000000000000577665577665000000277ee2277ee255665000000000000000000000000000000000000000000000000000005665
56665500000000000000000000000000000057666557666500000027eee227eee255665000000000000000000000000000000000000000000000000000005665
5666550000000000000000000000000000005666655666650000002eeee22eeee255665000000000000000006000666060606660600000000000000000005665
5666550000000000000000000000000000005666655666650000002eeee22eeee255665000000000000000006000600060606000600000000000000000005665
56665500000000000000000000000000000055555555555500000022222222222255665000000000000000006000660060606600600000000000000000005665
56665500000000000000000011111155555555555544444488888888888888888855665000000000000000006000600066606000600000000000000000005665
566655000000000000000000177cc1577665577665477ff4877ee8877ee8877ee855665000000000000000006660666006006660666000000000000000005665
56665500000000000000000017ccc157666557666547fff487eee887eee887eee855665000000000000000000000000000000000000000000000000000005665
5666550000000000000000001cccc15666655666654ffff48eeee88eeee88eeee855665000000000000000000000000000000000000000000000000000005665
5666550000000000000000001cccc15666655666654ffff48eeee88eeee88eeee855665000000000000000000000000000000000000000000000000000005665
56665500000000000000000011111155555555555544444488888888888888888855665000000000000000000000000000000000000000000000000000005665
56665500000000000011111111111100000044444444444444444455555588888855665000000000000000000000000077700000000000000000000000005665
566655000000000000177cc1177cc1000000477ff4477ff4477ff4577665877ee855665000000000000000000000000070700000000000000000000000005665
56665500000000000017ccc117ccc100000047fff447fff447fff457666587eee855665000000000000000000000000070700000000000000000000000005665
5666550000000000001cccc11cccc10000004ffff44ffff44ffff45666658eeee855665000000000000000000000000070700000000000000000000000005665
5666550000000000001cccc11cccc10000004ffff44ffff44ffff45666658eeee855665000000000000000000000000077700000000000000000000000005665
56665500000000000011111111111100000044444444444444444455555588888855665000000000000000000000000000000000000000000000000000005665
56665500000099999911111111111111111188888888888888888855555555555555665000000000000000000000000000000000000000000000000000005665
566655000000977aa9177cc1177cc1177cc1877ee8877ee8877ee857766557766555665000000000000000000000000000000000000000000000000000005665
56665500000097aaa917ccc117ccc117ccc187eee887eee887eee857666557666555665000000000000000000000000000000000000000000000000000005665
5666550000009aaaa91cccc11cccc11cccc18eeee88eeee88eeee856666556666555665500000000000000000000000000000000000000000000000000055665
5666550000009aaaa91cccc11cccc11cccc18eeee88eeee88eeee856666556666555666555555555555555555555555555555555555555555555555555556665
56665500000099999911111111111111111188888888888888888855555555555555666666666666666666666666666666666666666666666666666666666665
56665500000099999911111111111111111111111144444488888855555555555555666666666666666666666666666666666666666666666666666666666665
566655000000977aa9177cc1177cc1177cc1177cc1477ff4877ee857766557766555666666666666666666666666666666666666666666666666666666666665
56665500000097aaa917ccc117ccc117ccc117ccc147fff487eee857666557666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa91cccc11cccc11cccc11cccc14ffff48eeee856666556666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa91cccc11cccc11cccc11cccc14ffff48eeee856666556666555666666666666666666666666666666666666666666666666666666666665
56665500000099999911111111111111111111111144444488888855555555555555666666666666666666666666666666666666666666666666666666666665
56665500000099999988888811111111111144444444444444444455555555555555666666666666666666666666666666666666666666666666666666666665
566655000000977aa9877ee8177cc1177cc1477ff4477ff4477ff457766557766555666666666666666666666666666666666666666666666666666666666665
56665500000097aaa987eee817ccc117ccc147fff447fff447fff457666557666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa98eeee81cccc11cccc14ffff44ffff44ffff456666556666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa98eeee81cccc11cccc14ffff44ffff44ffff456666556666555666666666666666666666666666666666666666666666666666666666665
56665500000099999988888811111111111144444444444444444455555555555555666666666666666666666666666666666666666666666666666666666665
56665500000099999988888888888888888899999999999999999999999955555555666666666666666666666666666666666666666666666666666666666665
566655000000977aa9877ee8877ee8877ee8977aa9977aa9977aa9977aa957766555666666666666666666666666666666666666666666666666666666666665
56665500000097aaa987eee887eee887eee897aaa997aaa997aaa997aaa957666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa98eeee88eeee88eeee89aaaa99aaaa99aaaa99aaaa956666555666666666666666666666666666666666666666666666666666666666665
5666550000009aaaa98eeee88eeee88eeee89aaaa99aaaa99aaaa99aaaa956666555666666666666666666666666666666666666666666666666666666666665
56665500000099999988888888888888888899999999999999999999999955555555666666666666666666666666666666666666666666666666666666666665
56665555555555555555555555555555555555555555555555555555555555555556666666666666666666666666666666666666666666666666666666666665
56666555555555555555555555555555555555555555555555555555555555555556666666666666666666666666666666666666666666666666666666666665
55666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666655
05555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555550

__map__
0000000000000000000000030000010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000000000000000000000030000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__sfx__
00010000385502c7502b7402b7402b7402b7402b7402b7302b7302b7202b720277000310001100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000100001757017560175400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000100000f570195702157026570295602b550225301e530155000c5000c500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00010000105700e5700c5700817008170081000810004100041000110003100031000310003100031000310003100013000130001300013000130001300036000360003600036000000000000000000000000000
000300003e3302f3303c3202f3203b320323203931034310373103231037710327003770037700377003770037700377003770037700000000000000000000000000000000000000000000000000000000000000
000400003e3302f3303c3202f3203b320323203932034320373203233037330323303733032320373203232037320323103771032710377103271037710327003770032700000000000000000000000000000000
000700001e3401d3401c3401b3401a34018340163301533013330103300e3300b330071300b130011300b130011400b1300113001120011000110001100011000000000000000000000000000000000000000000
00080000153201c3302133023330283402c3402d3402d3302d3302d3202d3102d3002d30000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
01060000215402154021540215400000000000000000000021540215402154021540000000000000000000002154021540215402154000000000001c5401c540245422454224542245421d5401d5401d5401d540
010600001a5401a5401a5401a540000000000000000000001a5401a5401a5401a54000000000001a5402650026500265001a5401a54000000000001a5401a5401f5401f5401f5401f5401d5401d5401d5401d540
010600001c5401c5401c5401c5400000000000000000000018540185401854018540000000000000000000001d5401d5401d5401d54000000000001f5401f5401d5421d5421d5421d5421c5401c5401c5401c540
010600001a5401a5401a5401a540000000000000000000001a5401a5401a5401a540000000000000000000001a5401a5401a5401a540135400000015540000001a5421a5421a5421a5421f5401f5401f5401f540
010600002154021540215402154000000000000000000000215402154021540215400000000000000000000021540215402154021540000000000022540225402454224542245422454226540265402654026540
01060000215402154021540215400000000000215402154000000000002154000000215402d5000000000000215402d5000000000000215402154021540215401f5421f5421f5421f5421d5401d5401d5401d540
010600001c5401c5401c5401c5400000000000000000000018540185401854018540000000000000000000001d5401d5401d5401d54000000000001f5401f5401d5421d5421d5421d5421c5401c5401c5401c540
010600001a5401a5401a5401a540000000000000000000001a5401a5401a5401a540000000000000000000001a5401a5401a5401a540000000000000000000000000000000000000000000000000000000000000
01060000215402154021540215402154021540215402154021532215322153221532215322153221532215321d5401d5401d5401d5401d5401d5401d5401d5401d5321d5321d5321d5321d5321d5321d5321d532
010600001f5401f5401f5401f5401f5401f5401f5401f5401f5321f5321f5321f5321f5321f5321f5321f5321d5401d5401d5401d5401d5401d5401d5401d5401c5421c5421c5421c5421c5421c5421c5421c542
010600001a5401a5401a5401a5401a5401a5401a5401a5401a5321a5321a5321a5321a5321a5321a5321a53219540195401954019540195401954019540195401953219532195321953219532195321953219532
010600001c5401c5401c5401c540000000000000000000001c5401c5401c5401c5401c540000001c5400000000000000001c540000001c5401c5401c5401c5401f5421f5421f5421f5421f5421f5421f5421f542
01060000215402154021540215402154021540215402154021532215322153221532215322153221532215321d5401d5401d5401d5401d5401d5401d5401d5401d5321d5321d5321d5321d5321d5321d5321d532
010600001f5401f5401f5401f5401f5401f5401f5401f5401f5321f5321f5321f5321f5321f5321f5321f5321d5401d5401d5401d5401d5401d5401d5401d5401f5421f5421f5421f5421f5421f5421f5421f542
010600002154021540215402154021540215402154021540215322153221532215322153221532215322153224540245402454024540245402454024540245402453224532245322453224532245322453224532
010600002554225542255422554225542255422554225542255322553225532255322553225532255322553200000000000000000000000000000000000000000000000000000000000000000000000000000000
010600000903009030153000000015030150300000000000090300903000000000001503015030000000000009030090300000000000150301503000000000000903009030000000000015030150300000000000
010600000e0300e03000000000001a0301a03000000000000e0300e03000000000001a0301a03000000000000e0300e03000000000001a0301a03000000000000e0300e03000000000001a0301a0300000000000
01060000020300203000000000000e0300e0300000000000020300203000000000000e0300e0300000000000020300203000000000000e0300e0300000000000020300203000000000000e0300e0300000000000
010600000503005030000000000011030110300000000000050300503000000000001103011030000000000005030050300000000000110301103000000000000503005030000000000011030110300000000000
01060000000300003000000000000c0300c0300000000000000300003000000000000c0300c0300000000000000300003000000000000c0300c0300000000000000300003000000000000c0300c0300000000000
010600001503015030150000000015030150301003010030090300903000000000001503015030100301003009030090300000000000150301503010030100300903009030000000000015030150301803018030
010600001303013030100000000013030130300c0300c0300703007030000000000013030130300c0300c0300703007030000000000013030130300c0300c0300703007030090300903013030130301503015030
010600001103011030000001100011030110300c0300c0300503005030000000000011030110300c0300c0300503005030000000000011030110300c0300c0300503005030070300703011030110301303013030
010600001003010030000000000010030100300903009030040300403000000000001003010030090300903004030040300000000000100301003009030090300403004030090300903010030100301503015030
010600001003010030000000000010030100300903009030040300403000000000001003010030090300903004030040300000000000100301000010000000001003010030100301003010000100000900000000
01060000090300903000000000000903000000090300000009030090300c000000000903000000090300000009030090300000000000090300000009030000000903009030000000000009030000000903000000
010600000e0300e03000000000000e030000000e030000000e0300e03000000000000e030000000e030000000e0300e03000000000000e030000000e030000000e0300e03000000000000e030000000e03000000
010600000203002030000000000002030000000203000000020300203000000000000203000000020300000002030020300000000000020300000002030000000203002030000000000002030000000203000000
010600000503005030000000000005030000000503000000050300503005000000000503000000050300000005030050300000000000050300000005030000000503005030000000000005030000000503000000
010600000003000030000000000000030000000003000000000300003000000000000003000000000300000000030000300000000000000300000000030000000003000030000000000000030000000003000000
010600000703007030000000000007030000000703000000070300703000000000000703000000070300000007030070300000000000070300000007030000000703007030000000000007030000000703000000
010600000403004030000000000004030000000403000000040300403000000000000403000000040300000004030040300000000000040300000004030000000403004030000000000004030000000403000000
010600000403004030000000000004030000000403000000040300403000000000000403000000040300000004030040300000000000040300000004000000000403004030040300403004000000000400000000
__music__
00 41420818
00 41420919
00 41420a18
00 41420b1a
00 41420c1b
00 41420d1a
00 41420e1c
00 41420f1a
00 41420818
00 41420919
00 41420a18
00 41420b1a
00 41420c1b
00 41420d1a
00 41420e1c
00 41420f1a
00 4142101d
00 4142111e
00 4142121f
00 41421320
00 4142141d
00 4142151e
00 4142161f
00 41421721
00 41420822
00 41420923
00 41420a22
00 41420b24
00 41420c25
00 41420d24
00 41420e26
00 41420f24
00 41421022
00 41421127
00 41421225
00 41421328
00 41421422
00 41421527
00 41421625
02 41421729

