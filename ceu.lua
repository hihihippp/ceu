_CEU = true

_OPTS = {
    input     = nil,
    output    = '-',

    defs_file = '_ceu_defs.h',

    opt_join  = true,

    m4        = false,
    m4_args   = false,

    dfa       = false,
    dfa_viz   = false,
}

_OPTS_NPARAMS = {
    input       = nil,
    output      = 1,

    defs_file   = 1,

    opt_join    = 1,

    m4          = 0,
    m4_args     = 1,

    dfa         = 0,
    dfa_viz     = 0,
}

local params = {...}
local i = 1
while i <= #params
do
    local p = params[i]
    i = i + 1

    if p == '-' then
        _OPTS.input = '-'

    elseif string.sub(p, 1, 2) == '--' then
        local opt = string.gsub(string.sub(p,3), '%-', '_')
        if _OPTS_NPARAMS[opt] == 0 then
            _OPTS[opt] = true
        else
            _OPTS[opt] = params[i]
            i = i + 1
        end

    else
        _OPTS.input = p
    end
end
if not _OPTS.input then
    io.stderr:write([[

    ./ceu <filename>             # Ceu input file, or `-´ for stdin
    
        --output <filename>      # C output file (stdout)
    
        --defs-file <filename>   # defines constants & defines in a separate output file (no)
    
        --opt-join               # joins lines enclosed by /*{-{*/ and /*}-}*/ (true)
    
        --dfa                    # performs DFA analysis (false)
        --dfa-viz                # generates DFA graph (false)
    
        --m4                     # preprocesses the input with `m4´ (false)
        --m4-args                # preprocesses the input with `m4´ passing arguments in between `"´ (false)

]])
    os.exit(1)
end

-- INPUT
local inp
if _OPTS.input == '-' then
    inp = io.stdin
else
    inp = assert(io.open(_OPTS.input))
end
_STR = inp:read'*a'

if _OPTS.m4 or _OPTS.m4_args then
    local args = _OPTS.m4_args and string.sub(_OPTS.m4_args, 2, -2) or ''   -- remove `"´
    local m4_file = (_OPTS.input=='-' and '_tmp.ceu_m4') or _OPTS.input..'_m4'
    local m4 = assert(io.popen('m4 '..args..' - > '..m4_file, 'w'))
    m4:write(_STR)
    m4:close()

    _STR = assert(io.open(m4_file)):read'*a'
    --os.remove(m4_file)
end

-- PARSE
do
    dofile 'set.lua'

    dofile 'lines.lua'
    dofile 'parser.lua'
    dofile 'ast.lua'
    --ast.dump(ast.AST)
    dofile 'C.lua'
    dofile 'env.lua'
    dofile 'props.lua'
    dofile 'tight.lua'
    dofile 'exps.lua'
    dofile 'async.lua'
    dofile 'gates.lua'

    if _OPTS.dfa or _OPTS.dfa_viz then
        DBG('WRN : the DFA algorithm is exponential, this may take a while!')
        dofile 'nfa.lua'
        dofile 'dfa.lua'
        DBG('# States  ||  nfa: '.._NFA.n_nodes..'  ||  dfa: '.._DFA.n_states)
        if _OPTS.dfa_viz then
            dofile 'graphviz.lua'
        end
    end

    dofile 'code.lua'
end

-- TEMPLATE
local tpl
do
    tpl = assert(io.open'template.c'):read'*a'

    local sub = function (str, from, to)
        local i,e = string.find(str, from)
        return string.sub(str, 1, i-1) .. to .. string.sub(str, e+1)
    end

    tpl = sub(tpl, '=== N_WCLOCKS ===', #_GATES.wclocks)
    tpl = sub(tpl, '=== N_TRACKS ===', _AST.n_tracks)
    tpl = sub(tpl, '=== N_ASYNCS ===', _AST.n_asyncs)
    tpl = sub(tpl, '=== N_EMITS ===',  _AST.n_emits)
    tpl = sub(tpl, '=== N_GTES ===',   _GATES.n_gtes)
    tpl = sub(tpl, '=== N_ANDS ===',   _GATES.n_ands)
    tpl = sub(tpl, '=== TRGS ===',     table.concat(_GATES.trgs,','))
    tpl = sub(tpl, '=== N_VARS ===',   _ENV.n_vars)
    tpl = sub(tpl, '=== HOST ===',     _AST.host)
    tpl = sub(tpl, '=== CODE ===',     _AST.code)

    -- WCLOCKS
    do
        local wclocks = '{ '
        for i, gte in ipairs(_GATES.wclocks) do
            _GATES.wclocks[i] = '{ 0, 0, '..gte..' }'
        end
        tpl = sub(tpl, '=== WCLOCKS ===', table.concat(_GATES.wclocks,','))
    end

    -- LABELS
    do
        local labels = ''
        for i, id in ipairs(_CODE.labels) do
            -- i+1: (Inactive=0,Init=1,...)
            labels = labels..'    '..id..' = '..(i+1)..',\n'
        end
        tpl = sub(tpl, '=== LABELS ===', labels)
    end

    -- DEFINITIONS: constants & defines
    do
        -- EVENTS
        local str = ''
        local t = {}
        local outs = 0
        for id, evt in pairs(_ENV.exts) do
            if evt.input then
                str = str..'#define IN_'..id..' '..(evt.trg0 or 0)..'\n'
            else
                str = str..'#define OUT_'..id..' '..outs..'\n'
                outs = outs + 1
            end
        end
        str = str..'#define OUT_n '..outs..'\n'

        -- FUNCTIONS called
        for id in pairs(_EXPS.calls) do
            if id ~= '$anon' then
                str = str..'#define FUNC'..id..'\n'
            end
        end

        -- DEFINES
        if #_GATES.wclocks > 0 then
            str = str .. '#define CEU_WCLOCKS\n'
        end
        if _AST.n_asyncs > 0 then
            str = str .. '#define CEU_ASYNCS\n'
        end

        if _OPTS.defs_file then
            local f = io.open(_OPTS.defs_file,'w')
            f:write(str)
            f:close()
            tpl = sub(tpl, '=== DEFS ===',
                           '#include "'.. _OPTS.defs_file ..'"')
        else
            tpl = sub(tpl, '=== DEFS ===', str)
        end
    end
end

-- OUTPUT
local out
if _OPTS.output == '-' then
    out = io.stdout
else
    out = assert(io.open(_OPTS.output,'w'))
end
out:write(tpl)
