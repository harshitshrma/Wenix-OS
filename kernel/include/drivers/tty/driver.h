
#pragma once

struct tty_driver;
struct tty_device;

typedef void (*tty_driver_callback_t)(void *, char);

typedef struct tty_driver_ops {
        /**
         * Write the given character to the tty driver.
         *
         * @param ttyd the tty driver
         * @param c the character to write to the tty driver
         */
        void (*provide_char)(struct tty_driver *ttyd, char c);

        /**
         * Registers a callback to be called when the tty driver has
         * received a character from an input device and returns the
         * previous callback handler.
         *
         * @param ttyd the tty driver
         * @param callback the callback to register
         * @param arg the argument the callback will receive when called
         * @return returns the previous callback handler
         */
        tty_driver_callback_t (*register_callback_handler)(
                struct tty_driver *ttyd,
                tty_driver_callback_t callback,
                void *arg);

        /**
         * Unregisters the currently registered callback handler if
         * one exists and returns it.
         *
         * @param ttyd the tty driver
         * @return the current callback handler
         */
        tty_driver_callback_t (*unregister_callback_handler)(
                struct tty_driver *ttyd);

        /**
         * Blocks I/O from this driver. This guarantees that no
         * interrupts will be generated by this driver.
         *
         * @param ttyd the tty driver
         * @return implementation specific data that must be passed to
         * unblock_io().
         */
        void *(*block_io)(struct tty_driver *ttyd);

        /**
         * Unblocks I/O from this driver. This re-allows interrupts to
         * be generated by this driver.
         *
         * @param ttyd the tty driver
         * @param data the value returned by block_io()
         */
        void (*unblock_io)(struct tty_driver *ttyd, void *data);
} tty_driver_ops_t;

typedef struct tty_driver {
        tty_driver_ops_t     *ttd_ops;
        tty_driver_callback_t ttd_callback;
        void                 *ttd_callback_arg;
} tty_driver_t;