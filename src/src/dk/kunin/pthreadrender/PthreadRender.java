package dk.kunin.pthreadrender;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

public class PthreadRender extends Activity {

  static {
    System.loadLibrary("render");
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_pthread_render);

    findViewById(R.id.start).setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        startThreads();
      }
    });

    findViewById(R.id.stop).setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        stopThreads();
      }
    });
  }

  private void callback() {
    // TODO do context magic here

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Toast.makeText(getApplicationContext(), "Native callback" + System.nanoTime(), Toast.LENGTH_LONG).show();
      }
    });
  }

  private native void startThreads();
  private native void stopThreads();

}
