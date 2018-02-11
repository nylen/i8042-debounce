#include <linux/module.h>
#include <linux/serio.h>
#include <linux/vmalloc.h>
#include <linux/i8042.h>

#define KBD_DEBOUNCE_VERSION "0.1"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Debounce the i8042 keyboard");
MODULE_AUTHOR("James Nylen <jnylen@gmail.com>");
MODULE_VERSION(KBD_DEBOUNCE_VERSION);

#ifndef STANDARD_MSEC
#define STANDARD_MSEC 40
#endif

#ifndef MULTIKEY_MSEC
#define MULTIKEY_MSEC 55
#endif

#ifndef INTERKEY_MSEC
#define INTERKEY_MSEC 5
#endif

struct i8042_key_debounce_data {
	bool block_next_keyup;
	bool is_down;
	unsigned long jiffies_last_keyup;
};

#define NUM_KEYS 0x80
#define SIZEOF_KEYS (sizeof(struct i8042_key_debounce_data) * NUM_KEYS)
static struct i8042_key_debounce_data *keys;

#define KEYUP_MASK 0x80 // High bit: 1 (keyup), 0 (keydown)
#define KEYID_MASK 0x7f // Remaining 7 bits: which key
#define EXTENDED 0xe0   // E.g. 0xe0 0x1c (Keypad Enter)

static bool is_alpha_keydown(unsigned char x) {
	return (((x >= 0x10) && (x <= 0x19)) || // top row
			((x >= 0x1e) && (x <= 0x26)) || // middle row
			((x >= 0x2c) && (x <= 0x32)));  // bottom row
}

static bool i8042_debounce_filter(
	unsigned char data, // scancode
	unsigned char str,  // status register
	struct serio *serio // port
) {
	static bool extended;
	/* static unsigned char keys_currently_down; // TODO: find & fix the bug that breaks this count */
	static int keys_currently_down; // I suspect the decrement is dipping below zero
	static unsigned long jiffies_last_keydown;

	unsigned int msecs, msecs_since_keydown;
	struct i8042_key_debounce_data *key;

	if (serio->id.type != SERIO_8042_XL) { // Not a keyboard event
		return false;
	}

	pr_debug("i8042_debounce\n");
	pr_debug("i8042_debounce data=%02x\n", data);
	pr_debug("i8042_debounce\n");
	return false;

	if (unlikely(data == EXTENDED)) { // Extended keys
		extended = true;
		return false;
	} else if (unlikely(extended)) {
		extended = false;
		return false;
	}

	key = keys + (data & KEYID_MASK);

	if (data & KEYUP_MASK) {
		// keyup
		key->is_down = false;
		keys_currently_down--;
		if (keys_currently_down < 0) {
			pr_debug("i8042_debounce data=%02x kcd=%d NEGATIVE\n", data, keys_currently_down);
		}
		key->jiffies_last_keyup = jiffies;

		if (key->block_next_keyup) {
			key->block_next_keyup = false;
			return true;
		}
	} else {
		// keydown
		msecs = jiffies_to_msecs(jiffies - key->jiffies_last_keyup);

		if (!key->is_down) {
			key->is_down = true;
			keys_currently_down++;
		}

		if (unlikely((msecs < STANDARD_MSEC) || ((keys_currently_down > 1) && (msecs < MULTIKEY_MSEC)))) {
			// this key was released too recently
			pr_debug("i8042_debounce data=%02x ms=%u kcd=%d block (recent release)\n",
					 data, msecs, keys_currently_down);
			key->block_next_keyup = true;
			return true;

		} else if (likely(is_alpha_keydown(data) && (jiffies_last_keydown > 0))) {
			msecs_since_keydown = jiffies_to_msecs(jiffies - jiffies_last_keydown);
			if (unlikely(msecs_since_keydown < INTERKEY_MSEC)) {
				// another key was pressed *very* recently
				// nobody types that fast, so this is sympathetic bounce
				pr_debug("i8042_debounce data=%02x ms=%u kcd=%d block (very recent press)\n",
						 data, msecs, keys_currently_down);
				key->block_next_keyup = true;
				return true;
			}
		}

		pr_debug("i8042_debounce data=%02x ms=%u kcd=%d allow\n", data, msecs, keys_currently_down);

		jiffies_last_keydown = jiffies;
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

	keys = vmalloc(SIZEOF_KEYS);
	if (!keys) {
		pr_err("Unable to allocate memory\n");
		err = -ENOMEM;
		goto err_mem;
	}

	memset(keys, 0x00, SIZEOF_KEYS);

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
