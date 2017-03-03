#include <linux/module.h>
#include <linux/serio.h>
#include <linux/i8042.h>

#define KBD_DEBOUNCE_VERSION "0.1"

// vim: noet ts=4 sw=4

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Debounce the i8042 keyboard");
MODULE_AUTHOR("James Nylen <jnylen@gmail.com>");
MODULE_VERSION(KBD_DEBOUNCE_VERSION);

struct i8042_key_debounce_data {
	bool block_next_keyup;
	bool is_down;
	unsigned long jiffies_last_keyup;
};

struct i8042_key_debounce_data *keys;

static bool i8042_debounce_filter(
	unsigned char data, unsigned char str,
	struct serio *port
) {
	static bool extended;
	static unsigned char keys_currently_down;
	struct i8042_key_debounce_data *key;
	unsigned int msecs_since_keyup;

	if (port->id.type != SERIO_8042_XL) {
		// Not a keyboard event (touchpad, or maybe something else?)
		return false;
	}

	// Do not attempt to filter extended keys (0xe0 then another byte)
	if (unlikely(data == 0xe0)) {
		extended = true;
		return false;
	} else if (unlikely(extended)) {
		extended = false;
		return false;
	}

	key = keys + (data & 0x7f);

	if (data & 0x80) {
		// This is a keyup event
		key->is_down = false;
		keys_currently_down--;

		key->jiffies_last_keyup = jiffies;
		if (key->block_next_keyup) {
			key->block_next_keyup = false;
			pr_info(
				"i8042_debounce key=%02x up blocked\n",
				data & 0x7f
			);
			return true;
		}

		pr_debug(
			"i8042_debounce key=%02x up allowed\n",
			data & 0x7f
		);

	} else {
		// This is a keypress event
		// Block it and the next keyup if not enough time elapsed

		msecs_since_keyup = jiffies_to_msecs(jiffies - key->jiffies_last_keyup);
		if (!key->is_down) {
			key->is_down = true;
			keys_currently_down++;
		}
		if (unlikely(keys_currently_down > 1 && msecs_since_keyup < 50)) {
			pr_info(
				"i8042_debounce key=%02x press blocked ms=%u\n",
				data, msecs_since_keyup
			);
			key->block_next_keyup = true;
			return true;
		}

		// Show an extra message to help with tuning the delay
		if (unlikely(msecs_since_keyup < 100)) {
			pr_info(
				"i8042_debounce key=%02x press allowed ms=%u\n",
				data, msecs_since_keyup
			);
		} else {
			pr_debug(
				"i8042_debounce key=%02x press allowed ms=%u\n",
				data, msecs_since_keyup
			);
		}
	}

	return false;
}

static int __init i8042_debounce_init(void) {
	int ret, err;

	ret = i8042_install_filter(i8042_debounce_filter);
	if (ret) {
		pr_err("Unable to install key filter\n");
		err = -EBUSY;
		goto err_filter;
	}

	keys = vmalloc(sizeof(struct i8042_key_debounce_data) * 0x80);
	if (!keys) {
		pr_err("Unable to allocate memory\n");
		err = -ENOMEM;
		goto err_mem;
	}

	memset(keys, 0x00, sizeof(struct i8042_key_debounce_data) * 0x80);

	pr_info("i8042_debounce init\n");
	return 0;

err_mem:
	i8042_remove_filter(i8042_debounce_filter);
err_filter:
	return err;
}

static void __exit i8042_debounce_exit(void) {
	i8042_remove_filter(i8042_debounce_filter);
	vfree(keys);
	pr_info("i8042_debounce exit\n");
}

module_init(i8042_debounce_init);
module_exit(i8042_debounce_exit);
