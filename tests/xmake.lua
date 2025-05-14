add_requires("gtest")

local files = os.files("*_test.cpp")
for _, file in ipairs(files) do 
    -- 获取文件名（不包括路径和扩展名）
    local target_name = path.basename(file)
    target(target_name)
        set_kind("binary")
        add_packages("gtest")
        add_includedirs("../src", "../include")
        add_files(file)
        set_group("tests")
end