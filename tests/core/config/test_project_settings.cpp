/**************************************************************************/
/*  test_project_settings.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_project_settings)

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_pack.h"
#include "core/io/pck_packer.h"
#include "core/object/message_queue.h"
#include "core/variant/variant.h"
#include "tests/signal_watcher.h"
#include "tests/test_utils.h"

namespace TestProjectSettings {

TEST_CASE("[ProjectSettings] Get existing setting") {
	CHECK(ProjectSettings::get_singleton()->has_setting("application/run/main_scene"));

	Variant variant = ProjectSettings::get_singleton()->get_setting("application/run/main_scene");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, String());
}

TEST_CASE("[ProjectSettings] Default value is ignored if setting exists") {
	CHECK(ProjectSettings::get_singleton()->has_setting("application/run/main_scene"));

	Variant variant = ProjectSettings::get_singleton()->get_setting("application/run/main_scene", "SomeDefaultValue");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, String());
}

TEST_CASE("[ProjectSettings] Non existing setting is null") {
	CHECK_FALSE(ProjectSettings::get_singleton()->has_setting("not_existing_setting"));

	Variant variant = ProjectSettings::get_singleton()->get_setting("not_existing_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);
}

TEST_CASE("[ProjectSettings] Non existing setting should return default value") {
	CHECK_FALSE(ProjectSettings::get_singleton()->has_setting("not_existing_setting"));

	Variant variant = ProjectSettings::get_singleton()->get_setting("not_existing_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);

	variant = ProjectSettings::get_singleton()->get_setting("not_existing_setting", "my_nice_default_value");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, "my_nice_default_value");

	CHECK_FALSE(ProjectSettings::get_singleton()->has_setting("not_existing_setting"));
}

TEST_CASE("[ProjectSettings] Set value should be returned when retrieved") {
	CHECK_FALSE(ProjectSettings::get_singleton()->has_setting("my_custom_setting"));

	Variant variant = ProjectSettings::get_singleton()->get_setting("my_custom_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);

	ProjectSettings::get_singleton()->set_setting("my_custom_setting", true);
	CHECK(ProjectSettings::get_singleton()->has_setting("my_custom_setting"));

	variant = ProjectSettings::get_singleton()->get_setting("my_custom_setting");
	CHECK_EQ(variant.get_type(), Variant::BOOL);

	bool value = variant;
	CHECK_EQ(true, value);

	CHECK(ProjectSettings::get_singleton()->has_setting("my_custom_setting"));
}

TEST_CASE("[ProjectSettings] localize_path") {
	String old_resource_path = TestProjectSettingsInternalsAccessor::resource_path();
	TestProjectSettingsInternalsAccessor::resource_path() = DirAccess::create(DirAccess::ACCESS_FILESYSTEM)->get_current_dir();
	String root_path = ProjectSettings::get_singleton()->get_resource_path();
#ifdef WINDOWS_ENABLED
	String root_path_win = ProjectSettings::get_singleton()->get_resource_path().replace_char('/', '\\');
#endif

	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("filename"), "res://filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path/filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path/something/../filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path/./filename"), "res://path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path\\something\\..\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("path\\.\\filename"), "res://path/filename");
#endif

	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("../filename"), "../filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("../path/filename"), "../path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("..\\path\\filename"), "../path/filename");

	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("/testroot/filename"), "/testroot/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("/testroot/path/filename"), "/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("/testroot/path/something/../filename"), "/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("/testroot/path/./filename"), "/testroot/path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:/testroot/filename"), "C:/testroot/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:/testroot/path/filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:/testroot/path/something/../filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:/testroot/path/./filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:\\testroot\\filename"), "C:/testroot/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:\\testroot\\path\\filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:\\testroot\\path\\something\\..\\filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path("C:\\testroot\\path\\.\\filename"), "C:/testroot/path/filename");
#endif

	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path + "/filename"), "res://filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path + "/path/filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path + "/path/something/../filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path + "/path/./filename"), "res://path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path_win + "\\filename"), "res://filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path_win + "\\path\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path_win + "\\path\\something\\..\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::get_singleton()->localize_path(root_path_win + "\\path\\.\\filename"), "res://path/filename");
#endif

	TestProjectSettingsInternalsAccessor::resource_path() = old_resource_path;
}

TEST_CASE("[SceneTree][ProjectSettings] settings_changed signal") {
	SIGNAL_WATCH(ProjectSettings::get_singleton(), SNAME("settings_changed"));

	ProjectSettings::get_singleton()->set_setting("test_signal_setting", "test_value");
	MessageQueue::get_singleton()->flush();

	SIGNAL_CHECK("settings_changed", { {} });

	SIGNAL_UNWATCH(ProjectSettings::get_singleton(), SNAME("settings_changed"));
}

TEST_CASE("[ProjectSettings] get_changed_settings basic functionality") {
	String setting_name = "test_changed_setting";
	ProjectSettings::get_singleton()->set_setting(setting_name, "test_value");

	PackedStringArray changes = ProjectSettings::get_singleton()->get_changed_settings();
	CHECK(changes.has(setting_name));
}

TEST_CASE("[ProjectSettings] get_changed_settings multiple settings") {
	ProjectSettings::get_singleton()->set_setting("test_setting_1", "value1");
	ProjectSettings::get_singleton()->set_setting("test_setting_2", "value2");
	ProjectSettings::get_singleton()->set_setting("another_group/setting", "value3");

	PackedStringArray changes = ProjectSettings::get_singleton()->get_changed_settings();
	CHECK(changes.has("test_setting_1"));
	CHECK(changes.has("test_setting_2"));
	CHECK(changes.has("another_group/setting"));
}

TEST_CASE("[ProjectSettings] check_changed_settings_in_group") {
	ProjectSettings::get_singleton()->set_setting("group1/setting1", "value1");
	ProjectSettings::get_singleton()->set_setting("group1/setting2", "value2");
	ProjectSettings::get_singleton()->set_setting("group2/setting1", "value3");
	ProjectSettings::get_singleton()->set_setting("other_setting", "value4");

	CHECK(ProjectSettings::get_singleton()->check_changed_settings_in_group("group1/"));
	CHECK(ProjectSettings::get_singleton()->check_changed_settings_in_group("group2/"));
	CHECK_FALSE(ProjectSettings::get_singleton()->check_changed_settings_in_group("nonexistent/"));

	CHECK(ProjectSettings::get_singleton()->check_changed_settings_in_group("group1"));
	CHECK(ProjectSettings::get_singleton()->check_changed_settings_in_group("other_setting"));
}

TEST_CASE("[SceneTree][ProjectSettings] Changes cleared after settings_changed signal") {
	SIGNAL_WATCH(ProjectSettings::get_singleton(), SNAME("settings_changed"));

	ProjectSettings::get_singleton()->set_setting("signal_clear_test", "value");

	PackedStringArray changes_before = ProjectSettings::get_singleton()->get_changed_settings();
	CHECK(changes_before.has("signal_clear_test"));

	MessageQueue::get_singleton()->flush();

	SIGNAL_CHECK("settings_changed", { {} });

	PackedStringArray changes_after = ProjectSettings::get_singleton()->get_changed_settings();
	CHECK_FALSE(changes_after.has("signal_clear_test"));

	SIGNAL_UNWATCH(ProjectSettings::get_singleton(), SNAME("settings_changed"));
}

TEST_CASE("[ProjectSettings] No tracking when setting same value") {
	String setting_name = "same_value_test";
	String test_value = "same_value";

	ProjectSettings::get_singleton()->set_setting(setting_name, test_value);
	int count_before = ProjectSettings::get_singleton()->get_changed_settings().size();

	// Setting the same value should not be tracked due to early return.
	ProjectSettings::get_singleton()->set_setting(setting_name, test_value);
	int count_after = ProjectSettings::get_singleton()->get_changed_settings().size();

	CHECK_EQ(count_before, count_after);
}

// Helper: build a single-entry PCK containing `p_target` with `p_contents` bytes.
static String build_pck(const String &p_filename, const String &p_target, const String &p_contents) {
	const String pack_path = TestUtils::get_temp_path(p_filename);
	PCKPacker pck;
	REQUIRE(pck.pck_start(pack_path) == OK);
	REQUIRE(pck.add_file_from_buffer(p_target, p_contents.to_utf8_buffer()) == OK);
	REQUIRE(pck.flush() == OK);
	return pack_path;
}

static String read_packed_text(const String &p_path) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return String();
	}
	const uint64_t len = f->get_length();
	Vector<uint8_t> buf;
	buf.resize(len);
	f->get_buffer(buf.ptrw(), len);
	return String::utf8((const char *)buf.ptr(), len);
}

// `load_resource_pack` and `unload_resource_pack` are protected; route through
// the bound `Object::call` interface that the script API uses.
static bool ps_load_pack(const String &p_pack, bool p_replace_files = true) {
	return (bool)ProjectSettings::get_singleton()->call("load_resource_pack", p_pack, p_replace_files);
}

static bool ps_unload_pack(const String &p_pack, bool p_restore_files = true) {
	return (bool)ProjectSettings::get_singleton()->call("unload_resource_pack", p_pack, p_restore_files);
}

TEST_CASE("[ProjectSettings] unload_resource_pack basic") {
	const String target = "res://unload_basic/file.txt";
	const String pack_path = build_pck("unload_basic.pck", target, "hello");

	REQUIRE(ps_load_pack(pack_path));
	CHECK(PackedData::get_singleton()->has_path(target));

	CHECK(ps_unload_pack(pack_path));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));

	// Re-unload of an already-removed pack returns false.
	CHECK_FALSE(ps_unload_pack(pack_path));
}

TEST_CASE("[ProjectSettings] unload_resource_pack restores shadowed entry") {
	const String target = "res://unload_restore/file.txt";
	const String base_pack = build_pck("unload_restore_base.pck", target, "BASE");
	const String mod_pack = build_pck("unload_restore_mod.pck", target, "MOD!");

	REQUIRE(ps_load_pack(base_pack));
	REQUIRE(ps_load_pack(mod_pack, true));
	CHECK_EQ(read_packed_text(target), "MOD!");

	CHECK(ps_unload_pack(mod_pack, true));
	CHECK(PackedData::get_singleton()->has_path(target));
	CHECK_EQ(read_packed_text(target), "BASE");

	// Cleanup.
	CHECK(ps_unload_pack(base_pack));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));
}

TEST_CASE("[ProjectSettings] unload_resource_pack drops without restoring") {
	const String target = "res://unload_no_restore/file.txt";
	const String base_pack = build_pck("unload_no_restore_base.pck", target, "BASE");
	const String mod_pack = build_pck("unload_no_restore_mod.pck", target, "MOD!");

	REQUIRE(ps_load_pack(base_pack));
	REQUIRE(ps_load_pack(mod_pack, true));

	CHECK(ps_unload_pack(mod_pack, false));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));

	// Unloading the base pack must succeed (its contribution was kept in the
	// shadow stack and is still tracked) and leave a clean state.
	CHECK(ps_unload_pack(base_pack));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));
}

TEST_CASE("[ProjectSettings] unload_resource_pack handles stacked overrides") {
	const String target = "res://unload_stack/file.txt";
	const String pack_a = build_pck("unload_stack_a.pck", target, "AAAA");
	const String pack_b = build_pck("unload_stack_b.pck", target, "BBBB");
	const String pack_c = build_pck("unload_stack_c.pck", target, "CCCC");

	REQUIRE(ps_load_pack(pack_a));
	REQUIRE(ps_load_pack(pack_b, true));
	REQUIRE(ps_load_pack(pack_c, true));
	CHECK_EQ(read_packed_text(target), "CCCC");

	// Unload the middle of the stack: C remains active, B is spliced out
	// of the shadow stack, A is still pending below.
	CHECK(ps_unload_pack(pack_b, true));
	CHECK_EQ(read_packed_text(target), "CCCC");

	// Unloading C now must restore A (since B is gone from the stack).
	CHECK(ps_unload_pack(pack_c, true));
	CHECK_EQ(read_packed_text(target), "AAAA");

	CHECK(ps_unload_pack(pack_a));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));
}

TEST_CASE("[ProjectSettings] unload_resource_pack rejects mismatched pack path") {
	const String target = "res://unload_mismatch/file.txt";
	const String pack_path = build_pck("unload_mismatch.pck", target, "hello");

	REQUIRE(ps_load_pack(pack_path));

	// Insert `/./` before the filename: equivalent file system path but
	// not byte-identical to what was passed to `load_resource_pack`. The
	// contract requires an exact match.
	const int slash_pos = pack_path.rfind_char('/');
	REQUIRE(slash_pos > 0);
	const String mismatched = pack_path.left(slash_pos) + "/./" + pack_path.substr(slash_pos + 1);
	CHECK_FALSE(ps_unload_pack(mismatched));
	CHECK(PackedData::get_singleton()->has_path(target));

	CHECK(ps_unload_pack(pack_path));
	CHECK_FALSE(PackedData::get_singleton()->has_path(target));
}

TEST_CASE("[ProjectSettings] unload_resource_pack returns false for unknown pack") {
	CHECK_FALSE(ps_unload_pack(TestUtils::get_temp_path("never_loaded.pck")));
}

TEST_CASE("[ProjectSettings] unload_resource_pack rejects res://") {
	CHECK_FALSE(ps_unload_pack("res://"));
}

TEST_CASE("[PackedData] remove_pack drops delta contributions") {
	// Delta entries are added through PackSource paths that PCKPacker does
	// not expose, so drive `add_path` directly. The pack name is just an
	// identifier here; nothing tries to open the file.
	const String base_pack = "test://delta_base.pck";
	const String patch_pack = "test://delta_patch.pck";
	const String path = "delta_test/file.bin";
	uint8_t md5[16] = {};

	PackedData::get_singleton()->add_path(base_pack, path, 0, 0, md5, nullptr, false, false, false, false, "");
	PackedData::get_singleton()->add_path(patch_pack, path, 0, 0, md5, nullptr, false, false, false, true, "");

	CHECK(PackedData::get_singleton()->has_delta_patches(path));

	// Removing only the patch pack drops the delta but leaves the base entry.
	CHECK(PackedData::get_singleton()->remove_pack(patch_pack, true));
	CHECK_FALSE(PackedData::get_singleton()->has_delta_patches(path));
	CHECK(PackedData::get_singleton()->has_path(path));

	CHECK(PackedData::get_singleton()->remove_pack(base_pack, true));
	CHECK_FALSE(PackedData::get_singleton()->has_path(path));
}

} // namespace TestProjectSettings
