void notify_init(void)
{
    class_replaceMethod(objc_getClass("NSBundle"),
                        sel_registerName("bundleIdentifier"),
                        method_getImplementation((void *)^{return CFSTR("com.koekeishiya.skhd");}),
                        NULL);
}

void notify(const char *subtitle, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    CFStringRef format_ref = CFStringCreateWithCString(NULL, format, kCFStringEncodingUTF8);
    CFStringRef subtitle_ref = CFStringCreateWithCString(NULL, subtitle, kCFStringEncodingUTF8);
    CFStringRef message_ref = CFStringCreateWithFormatAndArguments(NULL, NULL, format_ref, args);

    void *center = objc_msgSend((void *) objc_getClass("NSUserNotificationCenter"), sel_registerName("defaultUserNotificationCenter"));
    void *notification = objc_msgSend((void *) objc_getClass("NSUserNotification"), sel_registerName("alloc"), sel_registerName("init"));

    objc_msgSend(notification, sel_registerName("setTitle:"), CFSTR("skhd"));
    objc_msgSend(notification, sel_registerName("setSubtitle:"), subtitle_ref);
    objc_msgSend(notification, sel_registerName("setInformativeText:"), message_ref);
    objc_msgSend(center, sel_registerName("deliverNotification:"), notification);

    CFRelease(message_ref);
    CFRelease(subtitle_ref);
    CFRelease(format_ref);

    va_end(args);
}
