-keep class org.libsdl.app.** { *; }

-keepclasseswithmembernames class * {
    native <methods>;
}

-keep public class * extends android.app.Activity
-keep public class * extends android.app.Application

-assumenosideeffects class android.util.Log {
    public static *** d(...);
    public static *** v(...);
}
