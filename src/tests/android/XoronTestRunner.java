package com.xoron.tests;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.ScrollView;
import android.util.Log;

/**
 * XoronTestRunner - Android Activity to run integration tests
 * Platform: Android 10+ (API 29+)
 */
public class XoronTestRunner extends Activity {
    
    private static final String TAG = "XoronTestRunner";
    private TextView logView;
    private ScrollView scrollView;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Create UI
        scrollView = new ScrollView(this);
        logView = new TextView(this);
        logView.setPadding(16, 16, 16, 16);
        logView.setTextAppearance(android.R.style.TextAppearance_Material_Small);
        logView.setText("Xoron Android Integration Tests\n\nStarting tests...\n");
        
        scrollView.addView(logView);
        setContentView(scrollView);
        
        // Run tests in background thread
        new Thread(() -> {
            try {
                runTests();
            } catch (Exception e) {
                Log.e(TAG, "Test error", e);
                appendLog("ERROR: " + e.getMessage());
            }
        }).start();
    }
    
    private void runTests() {
        appendLog("\n=== Xoron Android Integration Tests ===");
        appendLog("Platform: Android 10+ (API 29+)");
        appendLog("Date: 2026-01-06");
        appendLog("========================================\n");
        
        // Load native library
        try {
            System.loadLibrary("xoron_test_integration");
            appendLog("✓ Native library loaded");
        } catch (UnsatisfiedLinkError e) {
            appendLog("✗ Failed to load native library: " + e.getMessage());
            return;
        }
        
        // Run native tests
        appendLog("\nRunning native tests...\n");
        runNativeTests();
        
        appendLog("\n========================================");
        appendLog("Tests completed!");
        appendLog("Check logcat for detailed results");
        appendLog("========================================");
    }
    
    private native void runNativeTests();
    
    private void appendLog(final String message) {
        runOnUiThread(() -> {
            logView.append(message + "\n");
            // Auto-scroll to bottom
            scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
        });
    }
}
