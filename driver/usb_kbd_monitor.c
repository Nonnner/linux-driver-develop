/*
 * USB Keyboard Monitor - Input Handler Module
 * Demonstration of input subsystem driver for USB keyboard event capture on CentOS 7
 * 
 * This module registers as an input event handler to capture and log USB keyboard
 * key press/release events without modifying or blocking the default usbhid driver.
 * 
 * CentOS 7 Target: kernel ~3.10.x
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/version.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("USB Keyboard Monitor Project");
MODULE_DESCRIPTION("Input handler for USB keyboard event capture and logging (CentOS 7+)");

#define DRV_NAME "usb_kbd_monitor"

/*
 * Input handler event callback
 * Called whenever an input device (matching our target) generates an event
 */
static void usb_kbd_event(struct input_handle *handle, unsigned int type,
                          unsigned int code, int value)
{
	/* Log only keyboard events (EV_KEY) to reduce dmesg spam */
	if (type == EV_KEY) {
		const char *key_state;
		
		/* Map key state value to human-readable form */
		if (value == 1)
			key_state = "PRESSED";
		else if (value == 0)
			key_state = "RELEASED";
		else if (value == 2)
			key_state = "REPEAT";
		else
			key_state = "UNKNOWN";
		
		/* Log with standard prefix for easy filtering via dmesg */
		printk(KERN_INFO "%s: keycode=%u state=%s device=%s\n",
		       DRV_NAME, code, key_state, handle->dev->name);
	}
}

/*
 * Input handler connect callback
 * Called when a new input device appears that matches our criteria
 */
static int usb_kbd_connect(struct input_handler *handler, struct input_dev *dev,
                           const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;
	
	/* Allocate handle structure */
	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;
	
	/* Initialize handle fields */
	handle->name = DRV_NAME;
	handle->handler = handler;
	handle->dev = dev;
	
	/* Register the handle with input subsystem */
	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;
	
	/* Open the device to receive events */
	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;
	
	printk(KERN_INFO "%s: connected to device %s\n", DRV_NAME, dev->name);
	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	return error;
}

/*
 * Input handler disconnect callback
 * Called when an input device matching our criteria is removed
 */
static void usb_kbd_disconnect(struct input_handle *handle)
{
	printk(KERN_INFO "%s: disconnected from device %s\n",
	       DRV_NAME, handle->dev->name);
	
	/* Close the device and clean up */
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

/*
 * Device ID matching table
 * Specifies which input devices this handler should attach to
 * 
 * We target devices with EV_KEY capability (keyboards, input devices)
 * The driver= field allows optional filtering by driver name
 */
static const struct input_device_id usb_kbd_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
		/* Empty name means match any device with EV_KEY capability */
	},
	{ }  /* Terminator entry */
};

MODULE_DEVICE_TABLE(input, usb_kbd_ids);

/*
 * Input handler structure
 * Defines callbacks and matching criteria for the input event handler
 */
static struct input_handler usb_kbd_handler = {
	.event		= usb_kbd_event,
	.connect	= usb_kbd_connect,
	.disconnect	= usb_kbd_disconnect,
	.name		= DRV_NAME,
	.id_table	= usb_kbd_ids,
};

/*
 * Module initialization
 * Register the input handler when module is loaded
 */
static int __init usb_kbd_init(void)
{
	int error;
	
	printk(KERN_INFO "%s: initializing USB keyboard monitor (CentOS 7)\n", DRV_NAME);
	
	error = input_register_handler(&usb_kbd_handler);
	if (error) {
		printk(KERN_ERR "%s: failed to register input handler (error=%d)\n",
		       DRV_NAME, error);
		return error;
	}
	
	printk(KERN_INFO "%s: input handler registered, ready to capture keyboard events\n", DRV_NAME);
	return 0;
}

/*
 * Module cleanup
 * Unregister the input handler and clean up resources
 */
static void __exit usb_kbd_exit(void)
{
	printk(KERN_INFO "%s: unregistering input handler\n", DRV_NAME);
	input_unregister_handler(&usb_kbd_handler);
	printk(KERN_INFO "%s: cleanup complete\n", DRV_NAME);
}

module_init(usb_kbd_init);
module_exit(usb_kbd_exit);
