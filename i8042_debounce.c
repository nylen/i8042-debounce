#include <linux/module.h>
#include <linux/serio.h>
#include <linux/vmalloc.h>
#include <linux/i8042.h>

#define KBD_DEBOUNCE_VERSION "0.1"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Debounce the i8042 (Dell XPS) keyboard");
MODULE_AUTHOR("James Nylen <jnylen@gmail.com>, Ivan Brennan <ivan.brennan@gmail.com>");
MODULE_VERSION(KBD_DEBOUNCE_VERSION);

#ifdef STANDARD_MSEC
static const unsigned int standard_msec = STANDARD_MSEC;
#else
static const unsigned int standard_msec = 40;
#endif

#ifdef MULTIKEY_MSEC
static const unsigned int multikey_msec = MULTIKEY_MSEC;
#else
static const unsigned int multikey_msec = 55;
#endif

#ifndef INTERKEY_MSEC
static const unsigned int interkey_msec = INTERKEY_MSEC;
#else
static const unsigned int interkey_msec = 5;
#endif

struct i8042_key_debounce_data {
	bool block_next_keyup;
	bool is_down;
	unsigned long jiffies_last_keyup;
};

static struct i8042_key_debounce_data *keys;

static const unsigned int sizeof_keys = (sizeof(struct i8042_key_debounce_data) * 128);

static const unsigned char keyup_mask = 0x80;	// High bit: 1 (keyup), 0 (keydown)
static const unsigned char keyid_mask = 0x7f;	// Remaining 7 bits: which key
static const unsigned char extension  = 0xe0;	// E.g. 0xe0 0x1c (Keypad Enter)
static const unsigned char max_keyid  = 0x3a;	// Highest keyid I care to debounce: CapsLock

static bool is_letter(const unsigned char k) {
	return (((k >= 0x10) && (k <= 0x19)) ||	// Q - P (top row)
		((k >= 0x1e) && (k <= 0x26)) ||	// A - L (middle row)
		((k >= 0x2c) && (k <= 0x32)));	// Z - M (bottom row)
}

static bool i8042_debounce_filter(
	unsigned char data,	// scancode
	unsigned char str,	// status register
	struct serio *serio	// port
) {
	static bool extended;
	static unsigned char keys_currently_down;
	static unsigned long jiffies_last_keydown;

	unsigned int msecs, msecs_since_keydown;
	unsigned char keyid;
	struct i8042_key_debounce_data *key;

	if (serio->id.type != SERIO_8042_XL) { // Not a keyboard event
		return false;
	}

	if (unlikely(data == extension)) { // Extended keys
		extended = true;
		return false;
	} else if (unlikely(extended)) {
		extended = false;
		return false;
	}

	keyid = data & keyid_mask;

	if (unlikely(keyid == 0x2b)) { // Either \ or part of an Ack/ID sequence
		return false;
	}
	if (unlikely(keyid > max_keyid)) {
		return false;
	}

	key = keys + keyid;

	if (data & keyup_mask) {
		// keyup
		key->is_down = false;
		if (likely(keys_currently_down)) {
			keys_currently_down--;
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
			pr_debug("i8042_debounce data=%02x ms=%u block (recent release)\n", data, msecs);
			key->block_next_keyup = true;
			return true;

		} else if (likely(is_letter(keyid) && (jiffies_last_keydown > 0))) {
			msecs_since_keydown = jiffies_to_msecs(jiffies - jiffies_last_keydown);
			if (unlikely(msecs_since_keydown < INTERKEY_MSEC)) {
				// another key was pressed *very* recently
				// nobody types that fast, so this is sympathetic bounce
				pr_debug("i8042_debounce data=%02x ms=%u block (very recent press)\n", data, msecs);
				key->block_next_keyup = true;
				return true;
			}
		}

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

	keys = vmalloc(sizeof_keys);
	if (!keys) {
		pr_err("Unable to allocate memory\n");
		err = -ENOMEM;
		goto err_mem;
	}

	memset(keys, 0x00, sizeof_keys);

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
