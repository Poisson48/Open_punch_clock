package org.openpunchclock.app;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.view.WindowManager;

import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class Platform {

    public static final String CHANNEL_ID = "openpunchclock.sync";
    public static final String CHANNEL_VEILLE_ID = "openpunchclock.veille";
    private static final int NOTIFICATION_ID = 4545;
    private static final int PERMISSION_REQUEST = 4545;

    public static final String PREF_WIDGET = "openpunchclock_widget";
    public static final String EXTRA_PUNCH_ACTION = "punch_action";
    private static final String PREF_PENDING_PUNCH = "pending_punch_action";

    public static void createChannel(Context ctx) {
        if (ctx == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
            return;
        NotificationManager nm = ctx.getSystemService(NotificationManager.class);
        if (nm == null)
            return;

        if (nm.getNotificationChannel(CHANNEL_ID) == null) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID, "Time Clock", NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription("Rappels et synchronisation");
            nm.createNotificationChannel(channel);
        }

        if (nm.getNotificationChannel(CHANNEL_VEILLE_ID) == null) {
            NotificationChannel veille = new NotificationChannel(
                    CHANNEL_VEILLE_ID, "Veille sync",
                    NotificationManager.IMPORTANCE_LOW);
            veille.setDescription("Sync en arrière-plan");
            veille.setShowBadge(false);
            veille.enableLights(false);
            veille.enableVibration(false);
            veille.setSound(null, null);
            nm.createNotificationChannel(veille);
        }
    }

    public static void requestPermission(Context ctx) {
        if (!(ctx instanceof Activity) || Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
            return;
        Activity activity = (Activity) ctx;
        if (activity.checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            activity.requestPermissions(
                    new String[]{ android.Manifest.permission.POST_NOTIFICATIONS },
                    PERMISSION_REQUEST);
        }
    }

    public static void configurePush(Context ctx, String baseUrl, String[] topics,
                                     String deviceId) {
        PushService.configure(ctx, baseUrl, topics, deviceId);
    }

    public static void showNotification(Context ctx, String title, String body, long whenMs) {
        if (ctx == null)
            return;
        createChannel(ctx);
        NotificationManager nm = ctx.getSystemService(NotificationManager.class);
        if (nm == null)
            return;

        Notification.Builder builder = new Notification.Builder(ctx, CHANNEL_ID)
                .setSmallIcon(smallIcon(ctx))
                .setContentTitle(title)
                .setContentText(body)
                .setStyle(new Notification.BigTextStyle().bigText(body))
                .setAutoCancel(true);

        if (whenMs > 0) {
            builder.setWhen(whenMs);
            builder.setShowWhen(true);
        }

        Intent open = ctx.getPackageManager().getLaunchIntentForPackage(ctx.getPackageName());
        if (open != null) {
            open.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            builder.setContentIntent(PendingIntent.getActivity(
                    ctx, 0, open,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE));
        }

        nm.notify(NOTIFICATION_ID, builder.build());
    }

    public static boolean shareText(Context ctx, String text) {
        if (ctx == null)
            return false;
        try {
            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType("text/plain");
            send.putExtra(Intent.EXTRA_TEXT, text);
            Intent chooser = Intent.createChooser(send, "Partager");
            chooser.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(chooser);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public static boolean shareFile(Context ctx, String path, String mimeType) {
        if (ctx == null || path == null)
            return false;
        try {
            File file = new File(path);
            if (!file.isFile())
                return false;
            Uri uri = FileProvider.getUriForFile(ctx,
                    "org.openpunchclock.app.fileprovider", file);
            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType(mimeType != null && !mimeType.isEmpty() ? mimeType : "*/*");
            send.putExtra(Intent.EXTRA_STREAM, uri);
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Intent chooser = Intent.createChooser(send, "Partager");
            chooser.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(chooser);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public static boolean setClipboard(Context ctx, String text) {
        if (ctx == null)
            return false;
        try {
            ClipboardManager cm = ctx.getSystemService(ClipboardManager.class);
            if (cm == null)
                return false;
            cm.setPrimaryClip(ClipData.newPlainText("text", text));
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public static void updateWidget(Context ctx, boolean clockedIn, boolean onBreak,
                                    String timerText, String statusText) {
        if (ctx == null)
            return;
        SharedPreferences prefs = ctx.getSharedPreferences(PREF_WIDGET, Context.MODE_PRIVATE);
        prefs.edit()
                .putBoolean("clockedIn", clockedIn)
                .putBoolean("onBreak", onBreak)
                .putString("timer", timerText != null ? timerText : "00:00:00")
                .putString("status", statusText != null ? statusText : "")
                .apply();
        PunchWidgetProvider.refreshAll(ctx);
    }

    public static String consumeLaunchPunchAction(Context ctx) {
        if (ctx == null)
            return "";

        SharedPreferences prefs = ctx.getSharedPreferences(PREF_WIDGET, Context.MODE_PRIVATE);
        String pending = prefs.getString(PREF_PENDING_PUNCH, "");
        if (pending != null && !pending.isEmpty()) {
            prefs.edit().remove(PREF_PENDING_PUNCH).apply();
            return pending;
        }

        if (!(ctx instanceof Activity))
            return "";
        Activity activity = (Activity) ctx;
        Intent intent = activity.getIntent();
        if (intent == null)
            return "";
        String action = intent.getStringExtra(EXTRA_PUNCH_ACTION);
        if (action == null)
            action = "";
        intent.removeExtra(EXTRA_PUNCH_ACTION);
        return action;
    }

    public static void queuePunchAction(Context ctx, String action) {
        if (ctx == null || action == null || action.isEmpty())
            return;
        ctx.getSharedPreferences(PREF_WIDGET, Context.MODE_PRIVATE)
                .edit()
                .putString(PREF_PENDING_PUNCH, action)
                .apply();
    }

    public static void showToast(Context ctx, String text) {
        if (ctx == null || text == null || text.isEmpty())
            return;
        android.widget.Toast.makeText(ctx.getApplicationContext(), text,
                android.widget.Toast.LENGTH_SHORT).show();
    }

    public static boolean installApk(Context ctx, String apkPath) {
        if (ctx == null || apkPath == null)
            return false;

        File apk = new File(apkPath);
        if (!apk.isFile() || apk.length() == 0)
            return false;

        PackageInstaller.Session session = null;
        try {
            PackageInstaller installer = ctx.getPackageManager().getPackageInstaller();
            PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                    PackageInstaller.SessionParams.MODE_FULL_INSTALL);

            int sessionId = installer.createSession(params);
            session = installer.openSession(sessionId);

            try (InputStream in = new FileInputStream(apk);
                 OutputStream out = session.openWrite("openpunchclock", 0, apk.length())) {
                byte[] buffer = new byte[65536];
                int read;
                while ((read = in.read(buffer)) > 0)
                    out.write(buffer, 0, read);
                session.fsync(out);
            }

            Intent status = new Intent(ACTION_INSTALL_STATUS).setPackage(ctx.getPackageName());
            int flags = PendingIntent.FLAG_UPDATE_CURRENT;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
                flags |= PendingIntent.FLAG_MUTABLE;

            PendingIntent pending = PendingIntent.getBroadcast(ctx, sessionId, status, flags);
            session.commit(pending.getIntentSender());
            return true;

        } catch (Exception e) {
            if (session != null)
                session.abandon();
            return false;
        } finally {
            if (session != null)
                session.close();
        }
    }

    public static final String ACTION_INSTALL_STATUS = "org.openpunchclock.app.INSTALL_STATUS";

    public static class InstallReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context ctx, Intent intent) {
            int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS,
                                            PackageInstaller.STATUS_FAILURE);
            if (status != PackageInstaller.STATUS_PENDING_USER_ACTION)
                return;

            Intent confirm = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            if (confirm == null)
                return;
            confirm.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(confirm);
        }
    }

    public static void vibrate(Context ctx, int ms) {
        if (ctx == null)
            return;
        try {
            Vibrator vibrator;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                VibratorManager manager = ctx.getSystemService(VibratorManager.class);
                vibrator = (manager != null) ? manager.getDefaultVibrator() : null;
            } else {
                vibrator = (Vibrator) ctx.getSystemService(Context.VIBRATOR_SERVICE);
            }
            if (vibrator == null || !vibrator.hasVibrator())
                return;
            vibrator.vibrate(VibrationEffect.createOneShot(
                    ms, VibrationEffect.DEFAULT_AMPLITUDE));
        } catch (Exception e) {
            // optional
        }
    }

    public static void keepScreenOn(Context ctx, final boolean on) {
        if (!(ctx instanceof Activity))
            return;
        final Activity activity = (Activity) ctx;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (on)
                    activity.getWindow().addFlags(
                            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                else
                    activity.getWindow().clearFlags(
                            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });
    }

    private static int smallIcon(Context ctx) {
        int id = ctx.getResources().getIdentifier(
                "ic_stat_notify", "drawable", ctx.getPackageName());
        return id != 0 ? id : android.R.drawable.stat_notify_sync;
    }
}
