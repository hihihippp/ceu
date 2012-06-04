local simul = require 'nesc'

for i=0, 6 do
    _G['n'..i] = simul.app {
				values = {
					Photo_readDone = { i, },
				},
        defines = {
            TOS_NODE_ID = i,
            TOS_COLLISION = 90,
		    SERVER_ID = 0,
        },
        source = assert(io.open'../samples/probeEchoM4.ceu'):read'*a',
    }
end

simul.topology {
    [n0]  = { n1, },
    [n1]  = { n0, n2, n4, n6, },
    [n2]  = { n1, n3, n5},
    [n3]  = { n2, n4, n6, },
    [n4]  = { n1, n3, n5, },
    [n5]  = { n2, n4, n6, },
    [n6]  = { n1, n3, n5, },
}   


simul.shell()