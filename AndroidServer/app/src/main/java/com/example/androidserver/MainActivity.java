package com.example.androidserver;

import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Thread connectionListenerThread = new Thread(new Server());
        connectionListenerThread.start();

        Log.d("mainThread", "checkpoint");

        try {
            Log.d("mainThread", "Blocking until connectionListenerThread finishes...");
            connectionListenerThread.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}