#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Keep manual and Store pack size walks outside the UI snapshot lock."""
def compose(text):
    old='''        {
            std::scoped_lock lock(g_mutex);
            g_imported_at[StableId(destination)] = CurrentTimestamp();
            g_managed_sizes[StableId(destination)] = ManagedSize(destination);
            SaveSettingsLocked();
        }'''
    new='''        const auto imported_size = ManagedSize(destination);
        const auto imported_stamp = CurrentTimestamp();
        {
            std::scoped_lock lock(g_mutex);
            g_imported_at[StableId(destination)] = imported_stamp;
            g_managed_sizes[StableId(destination)] = imported_size;
            SaveSettingsLocked();
        }'''
    # Rice uses an inner branch, RTZ uses the function's outer scope.
    for indent in ('        ','    '):
        before=old if indent=='        ' else '\n'.join(line[4:] if line.startswith('    ') else line for line in old.split('\n'))
        after=new if indent=='        ' else '\n'.join(line[4:] if line.startswith('    ') else line for line in new.split('\n'))
        if before in text:text=text.replace(before,after)
    before='''    const auto id = StableId(path);
    {
        std::lock_guard lock(g_mutex);
        g_imported_at.try_emplace(id, CurrentTimestamp());
        g_managed_sizes[id] = ManagedSize(path);'''
    after='''    const auto id = StableId(path);
    const auto imported_size = ManagedSize(path);
    {
        std::lock_guard lock(g_mutex);
        g_imported_at.try_emplace(id, CurrentTimestamp());
        g_managed_sizes[id] = imported_size;'''
    return text.replace(before,after)
