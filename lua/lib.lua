function keys(tab)
    local arr,n={},0
    for k,v in pairs(tab) do
        n=n+1
        arr[n]=k
    end
    return arr
end
