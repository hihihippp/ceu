local VARS = {}

if not _ANA then
    return          -- isTmp=false for all vars
end

F = {
    Dcl_var_pre = function (me)
        local var = me.var
        if var.isEvt or var.cls or var.inIfc then
            return                  -- only normal vars can be tmp
        end
        VARS[var] = true
        var.isTmp = true
    end,

    Var = function (me)
        local var = me.var
        if var.isEvt or var.cls or var.inIfc then
            return                  -- only normal vars can be tmp
        end
        if _AST.iter'Dcl_var'() or
           me.__par.tag == 'SetBlock' then
            return                  -- dcl is not an access
        end

        local v = VARS[var]

        local op = _AST.iter'Op1_&'()
        local isRef = op and (op.ref == me)

        if _AST.iter'Finally'() or      -- finally executes through "call"
           _AST.iter'AwaitInt'() or     -- await ptr:a (ptr is tested on awake)
           isRef                        -- reference may escape
        then
            var.isTmp = false
            VARS[var] = nil
            return
        end

        if v == true then
            VARS[var] = me.ana.pre
            return                  -- first access
        end

        if not (v and _ANA.CMP(v,me.ana.pre)) then
DBG('no', var.id)
            var.isTmp = false       -- found a Par or Await in the path
            return
        end
    end,

    Loop_pre = function (me)
        if me.noAwts or _AST.iter'SetAwait'() then
            return  -- (tight loop) or (await ... until)
        end
        VARS = {}       -- a loop in between Dcl/Accs is dangerous
    end,

    ParOr_pre = function (me)
        for var, v in pairs(VARS) do
            if v ~= true then
                VARS[var] = nil     -- remove previously accessed vars
            end
        end
    end,
    ParAnd_pre  = 'ParOr_pre',
    ParEver_pre = 'ParOr_pre',
    ParOr   = 'ParOr_pre',
    ParAnd  = 'ParOr_pre',
    ParEver = 'ParOr_pre',

    -- TODO: should pre's be already different?
    Async_pre = 'ParOr_pre',
    Async     = 'ParOr_pre',
}

_AST.visit(F)