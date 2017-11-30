close = function ()
    local pControl = utility.get_ctrl_ptr_by_cid(CID_HELPER_REPAIR_DLG);
    pControl:Show(FALSE);
    --nControl很可能出现拼写错误
    layout_mgr.delay_remove_ctrl(nControl);
end;