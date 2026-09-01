package org.openpunchclock.app;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.widget.RemoteViews;

public class PunchWidgetProvider extends AppWidgetProvider {

    public static final String ACTION_PUNCH_IN = "org.openpunchclock.app.PUNCH_IN";
    public static final String ACTION_PUNCH_OUT = "org.openpunchclock.app.PUNCH_OUT";

    @Override
    public void onUpdate(Context context, AppWidgetManager manager, int[] appWidgetIds) {
        SharedPreferences prefs = context.getSharedPreferences(Platform.PREF_WIDGET, Context.MODE_PRIVATE);
        final boolean clockedIn = prefs.getBoolean("clockedIn", false);
        final boolean onBreak = prefs.getBoolean("onBreak", false);
        final String timer = prefs.getString("timer", "00:00:00");
        final String status = prefs.getString("status", "");

        for (int id : appWidgetIds)
            updateWidget(context, manager, id, clockedIn, onBreak, timer, status);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);
        if (intent == null || intent.getAction() == null)
            return;

        String punch = null;
        if (ACTION_PUNCH_IN.equals(intent.getAction()))
            punch = "in";
        else if (ACTION_PUNCH_OUT.equals(intent.getAction()))
            punch = "out";

        if (punch != null)
            launchApp(context, punch);
    }

    private static void launchApp(Context context, String punchAction) {
        Platform.queuePunchAction(context, punchAction);
        Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (launch == null)
            return;
        launch.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        launch.putExtra(Platform.EXTRA_PUNCH_ACTION, punchAction);
        context.startActivity(launch);
    }

    public static void refreshAll(Context context) {
        AppWidgetManager manager = AppWidgetManager.getInstance(context);
        ComponentName cn = new ComponentName(context, PunchWidgetProvider.class);
        int[] ids = manager.getAppWidgetIds(cn);
        if (ids.length == 0)
            return;

        SharedPreferences prefs = context.getSharedPreferences(Platform.PREF_WIDGET, Context.MODE_PRIVATE);
        final boolean clockedIn = prefs.getBoolean("clockedIn", false);
        final boolean onBreak = prefs.getBoolean("onBreak", false);
        final String timer = prefs.getString("timer", "00:00:00");
        final String status = prefs.getString("status", "");

        for (int id : ids)
            updateWidget(context, manager, id, clockedIn, onBreak, timer, status);
    }

    private static void updateWidget(Context context, AppWidgetManager manager, int widgetId,
                                   boolean clockedIn, boolean onBreak,
                                   String timer, String status) {
        RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.widget_punch);

        if (status != null && !status.isEmpty())
            views.setTextViewText(R.id.widget_status, status);
        else if (onBreak)
            views.setTextViewText(R.id.widget_status, context.getString(R.string.widget_status_break));
        else if (clockedIn)
            views.setTextViewText(R.id.widget_status, context.getString(R.string.widget_status_working));
        else
            views.setTextViewText(R.id.widget_status, context.getString(R.string.widget_status_ready));

        views.setTextViewText(R.id.widget_timer, timer != null ? timer : "00:00:00");

        views.setOnClickPendingIntent(R.id.widget_btn_in,
                pendingAction(context, ACTION_PUNCH_IN, 1));
        views.setOnClickPendingIntent(R.id.widget_btn_out,
                pendingAction(context, ACTION_PUNCH_OUT, 2));

        manager.updateAppWidget(widgetId, views);
    }

    private static PendingIntent pendingAction(Context context, String action, int reqCode) {
        Intent intent = new Intent(context, PunchWidgetProvider.class);
        intent.setAction(action);
        int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
        return PendingIntent.getBroadcast(context, reqCode, intent, flags);
    }
}
