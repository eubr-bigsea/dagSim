--
-- Lua support function library
--
solver = {  -- the entire library to interface with the solver
};

solver.serpent = require("serpent");
function solver.serialize()
	return solver.serpent.dump(_G, {valignore = {[bit32] = true, [collectgarbage] = true,
		[coroutine] = true, [debug] = true, [_exe] = true, [assert] = true, [dofile] = true,
		[_VERSION] = true, [error] = true, [getmetatable] = true, [io] = true, [ipairs] = true,
		[load] = true, [loadfile] = true, [math] = true, [next] = true, [os] = true,
		[package] = true, [pairs] = true, [pcall] = true, [print] = true, [rawequal] = true,
		[rawget] = true, [rawlen] = true, [rawset] = true, [require] = true, [select] = true,
		[setmetatable] = true, [solver] = true, [_exe] = true, [assert] = true, [dofile] = true,
		[_G] = true, [string] = true, [table] = true, [tonumber] = true, [tostring] = true,
		[type] = true, [utf8] = true, [xpcall] = true
		}});
end


--
solver.args = _exe.args;	-- puts the command line arguments in the solver object
--
function solver.solve()
	return _exe.solve();
end
--
function solver.multisolve(nt, L)
	_exe.maxThreads = nt;
	return _exe.multisolve(L);
end
--
function solver.printRes(R)	
	local row = 1;
	while (R[row]) do
		local col = 1;
		local out = "";
		while (R[row][col]) do
			out = out .. R[row][col] .. "\t";
			col = col + 1;
		end
		print(out);
		row = row + 1;
	end
end
--
function solver.printMultiRes(R)
	local expid = 1;
	while (R[expid]) do
		local row = 1;
		while (R[expid][row]) do
			local col = 1;
			local out = expid.."\t";
			while (R[expid][row][col]) do
				out = out .. R[expid][row][col] .. "\t";
				col = col + 1;
			end
			print(out);
			row = row + 1;
		end
		expid = expid + 1;
	end
end
--
function solver.writeRes(R, filename)
	solver.writeResFiltered(R, filename, function(row) return true end);
end
--
function solver.writeResFiltered(R, filename, filter)
	solver.writeResFilteredAction(R, filename, filter, function(row) return "" end);
end
---
function solver.writeResFilteredAction(R, filename, filter, action)
	local row = 1;
	local f = assert(io.open(filename, "w"));
	while (R[row]) do
		if(filter(R[row])) then
			local col = 1;
			local out = "";
			while (R[row][col]) do
				out = out .. R[row][col] .. "\t";
				col = col + 1;
			end
			out = out .. "\n" .. action(R[row]);
			f:write(out);
		end
		row = row + 1;
	end
	f:close();
end
--
function solver.main()	-- default main solution component
	solver.printRes(solver.solve());
end
--
function solver.fileToArray(fileName)
	local file = io.open(fileName)
	local tbllines = {}
	local i = 0
	if file then
	    for line in file:lines() do
	     i = i + 1
	     tbllines[i] = line
	    end
	    file:close()
	else
	    error('file not found')
	end
	return tbllines;
end
---
function solver.makeHistogram(dataSet)
	table.sort(dataSet,function(a,b) return (a-0)<(b-0) end);
	local minV = dataSet[1];
	local maxV = dataSet[#dataSet];
	local i;
	
	local q = {};
	for i = 1,3 do
		local qp = (#dataSet-1) * i / 4;
		local iqp = math.floor(qp);
		local aqp = qp-iqp;
		
		if (iqp < 0) then
			q[i] = minV;
		elseif(iqp >= #dataSet-1) then
			q[i] = maxV;
		else
			q[i] = dataSet[iqp+1] * (1 - aqp) + dataSet[iqp + 2] * aqp;
		end 
	end
	local binSize = 2 * (q[3]-q[2]) / math.pow(#dataSet, 1/3);
	local firstBin = math.floor(minV / binSize);
	local lastBin  = math.floor(maxV / binSize);
	
	local h = {};
	for i=firstBin,lastBin do
		h[i-firstBin+1] = 0;
	end
	for i=1,#dataSet do
		local bix = math.floor(dataSet[i] / binSize) - firstBin + 1;
		h[bix] = h[bix] + 1;
	end
	return {w = h, size = binSize, min = firstBin};
end