_TP = {}

local types = {
    void=true, char=true, byte=true, bool=true, word=true,
    uint=true, int=true,
    u64=true,  s64=true,
    u32=true,  s32=true,
    u16=true,  s16=true,
    u8=true,   s8=true,
    float=true,
    f32=true,  f64=true,
}

-- len aligned to word size
function _TP.sizeof (len)
    local al = len
    if al > _ENV.c.word.len then
        al = _ENV.c.word.len   -- maximum adjust is the word size
    end
    local r = len % al
    if r > 0 then
        len = len + (al-r)
    end
    return len
end

-- returns off/aligned + len
function _TP.align (off, len)
    if len > _ENV.c.word.len then
        len = _ENV.c.word.len   -- maximum adjust is the word size
    elseif len == 0 then
        len = 1                 -- minimum alignment (TODO: why?)
    end
    local r = off % len
    if r > 0 then
        off = off + (len-r)
    end
    return off
end


function _TP.n2bytes (n)
    if n < 2^8 then
        return 1
    elseif n < 2^16 then
        return 2
    elseif n < 2^32 then
        return 4
    end
    error'out of bounds'
end

function _TP.ceil (v)
    local w = _OPTS.tp_word
    while true do
        if v % w == 0 then
            return v
        else
            v = v + 1
        end
    end
end

-- TODO: enforce passing parameter `c´ to isNumeric/deref/contains/max ?

function _TP.noptr (tp)
    return (string.match(tp, '^([^%*]*)%**'))
end

--[[
function _TP.c (tp)
    -- _tp->tp
    -- class->CEU_XXX
    -- class*->CEU_XXX* (too!)
    return (string.gsub(string.gsub(tp,'^%u[_%w]*%*?','char*'), '^_', ''))
end
]]

function _TP.isTuple (tp,ptr)
    return _ENV.c[tp] and _ENV.c[tp].tuple
        or _TP.deref(tp) and _TP.isTuple(_TP.deref(tp))
end

function _TP.c (tp)
    local cls = _ENV.clss[_TP.noptr(tp)]
    if cls then
        return 'CEU_'..tp
    end

    return (string.gsub(tp,'^_', ''))
end

function _TP.isNumeric (tp, c)
    return tp~='void' and types[tp] or (c and _TP.ext(tp,c))
end

function _TP.deref (tp, c)
    return string.match(tp,'(.-)%*$') or (c and _TP.ext(tp,c))
end

function _TP.ext (tp, loc)
    return (tp=='_' and '_') or
           (loc and (not _TP.deref(tp)) and (string.sub(tp,1,1) == '_') and tp)
end

function _TP.contains (tp1, tp2, c)
    -- same exact type
    if tp1 == tp2 then
        return true
    end

    -- both are numeric
    if _TP.isNumeric(tp1,c) and _TP.isNumeric(tp2,c) then
        return true
    end

    -- both are pointers
    local _tp1, _tp2 = _TP.deref(tp1,c), _TP.deref(tp2,c)
    if _tp1 and _tp2 then
        local cls1 = _ENV.clss[_tp1]
        local cls2 = _ENV.clss[_tp2]
        -- assigning to a cls (cast is enforced)
        if cls1 then
            return tp2 == 'null*' or
                   cls2 and cls1.is_ifc and _ENV.ifc_vs_cls(cls1,cls2)
        end
        return tp1=='void*' or tp2=='void*' or tp2=='null*'
                or c and (tp1=='_' or tp2=='_')
                or _TP.contains(_tp1, _tp2, c)
    end

    -- c=accept ext // and at least one is ext
    if c and (_TP.ext(tp1) or _TP.ext(tp2)) then
        return true
    end

    -- tuples vs (tuples or single types)
    local tup1 = _TP.isTuple(tp1)
    local tup2 = _TP.isTuple(tp2)
    if tup1 or tup2 then
        tup1 = tup1 or { {nil,tp1,nil} }
        tup2 = tup2 or { {nil,tp1,nil} }
        if #tp1 == #tp2 then
            for i=1, #tp1 do
                local hold, tp1, id = unpack(tp1[i])
                local hold, tp2, id = unpack(tp1[i])
                if not _TP.contains(tp1,tp2) then
                    return false
                end
            end
        end
        return true
    end

    return false
end

function _TP.max (tp1, tp2, c)
    if _TP.contains(tp1, tp2, c) then
        return tp1
    elseif _TP.contains(tp2, tp1, c) then
        return tp2
    else
        return nil
    end
end
