void notify_init(void)
{
    class_replaceMethod(objc_getClass("NSBundle"),
                        sel_registerName("bundleIdentifier"),
                        method_getImplementation((void *)^{return CFSTR("com.koekeishiya.skhd");}),
                        NULL);
}

void notify(char *title, char *message)
{
    CFStringRef title_ref = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
    CFStringRef message_ref = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);

    void *center = objc_msgSend((void *) objc_getClass("NSUserNotificationCenter"), sel_registerName("defaultUserNotificationCenter"));
    void *notification = objc_msgSend((void *) objc_getClass("NSUserNotification"), sel_registerName("alloc"), sel_registerName("init"));

    objc_msgSend(notification, sel_registerName("setTitle:"), title_ref);
    objc_msgSend(notification, sel_registerName("setInformativeText:"), message_ref);
    objc_msgSend(center, sel_registerName("deliverNotification:"), notification);

    CFRelease(message_ref);
    CFRelease(title_ref);
}
