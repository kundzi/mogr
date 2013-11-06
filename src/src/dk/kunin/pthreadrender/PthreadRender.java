package dk.kunin.pthreadrender;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

public class PthreadRender extends Activity implements Callback {

  static {
    System.loadLibrary("render");
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_pthread_render);
    mSurfaceView = (SurfaceView) findViewById(R.id.surface);
    mSurfaceView.getHolder().addCallback(this);

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
//    createContext();
  }


  private SurfaceView mSurfaceView;
  private SurfaceHolder mHolder;

  private volatile EGLConfig  mConfig;
  final int[] mContextAttributes = new int[] {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL11.EGL_NONE
  };

  private volatile EGLDisplay mEglDisplay;
  private volatile EGLSurface mEglSurface;

  private volatile EGLContext mEglContext1;
  private volatile EGLContext mEglContext2;

  private final Object SYNC = new Object();

  private final EGL10 egl = (EGL10) EGLContext.getEGL();

  private void createContext()
  {
    synchronized (SYNC)
    {
      if (mEglContext1 == null)
      {
        // Display
        mEglDisplay = egl.eglGetDisplay(EGL11.EGL_DEFAULT_DISPLAY);
        _assert(mEglDisplay != EGL11.EGL_NO_DISPLAY,
            "Can get display: " + getErrorName(getEglError()));

        final int[] version = new int[2];
        _assert(egl.eglInitialize(mEglDisplay, version),
            "EGL Initialization error: " + getErrorName(getEglError()));


        // Surface + Context
        final EGLConfig[] configs = getConfigs();
        for (final EGLConfig config : configs)
        {
          mEglSurface = createSurface(config);
          if (mEglSurface == EGL11.EGL_NO_SURFACE)
            continue;


          mEglContext1 = egl.eglCreateContext(
              mEglDisplay, config, EGL11.EGL_NO_CONTEXT, mContextAttributes);
          if (mEglContext1 == EGL11.EGL_NO_CONTEXT)
          {
            egl.eglDestroySurface(mEglDisplay, mEglSurface);
            continue;
          }
          mConfig = config;
          break;
        }


        _assert(mEglSurface != EGL11.EGL_NO_SURFACE,
            "No surface created: " + getErrorName(getEglError()));

        _assert(mEglContext1 != EGL11.EGL_NO_CONTEXT,
            "First context not created: " + getErrorName(getEglError()));

        //make current
        _assert(egl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext1),
            "Cant make first context current: "+ getErrorName(getEglError()));

        log("First context OK");
      }
      else if (mEglContext2 == null)
      {
        // share context with first one
        mEglContext2 = egl.eglCreateContext(
            mEglDisplay, mConfig, mEglContext1, mContextAttributes);

        _assert(mEglContext2 != EGL11.EGL_NO_CONTEXT,
            "Second context not created: " + getErrorName(getEglError()));

        log("Creating second surface");

        final EGLSurface eglSurface2 = EGL11.EGL_NO_SURFACE;//createSurface(mConfig);

        log("Currenting second surface");
        _assert(egl.eglMakeCurrent(mEglDisplay, eglSurface2, eglSurface2, mEglContext2),
            "Cant make second context current: "+ getErrorName(getEglError()));

        log("Second context OK");
      }
    }
  }

  private static String getErrorName(int code)
  {
    final Field[] fields = EGL10.class.getFields();
    final List<Field> sfFields = new LinkedList<Field>();

    for (final Field f : fields)
    {
      if (f.getType() == Integer.class
          && Modifier.isFinal(f.getModifiers())
          && Modifier.isFinal(f.getModifiers()))
      {
        sfFields.add(f);
      }
    }

    // find name by value
    for (final Field f : sfFields)
    {
      try {
        if (f.getInt(null) == code)
          return code + " " + f.getName();
      } catch (final IllegalAccessException e) {
        e.printStackTrace();
      } catch (final IllegalArgumentException e) {
        e.printStackTrace();
      }
    }

    return code + " NOT FOUND";
  }

  private native void startThreads();
  private native void stopThreads();

  class ConfigSorter implements Comparator<EGLConfig>
  {
    @Override
    public int compare(EGLConfig lhs, EGLConfig rhs)
    {
      return getWeight(lhs) - getWeight(rhs);
    }

    private int getWeight(EGLConfig config)
    {
      final int[] value = new int[1];
      egl.eglGetConfigAttrib(mEglDisplay, config, EGL11.EGL_CONFIG_CAVEAT, value);

      switch (value[0])
      {
        case EGL11.EGL_NONE:
          return 0;
        case EGL11.EGL_SLOW_CONFIG:
          return 1;
        case EGL11.EGL_NON_CONFORMANT_CONFIG:
          return 2;
      }

      return 0;
    }
  }

  private EGLSurface createSurface(EGLConfig config)
  {
    return egl.eglCreateWindowSurface(
        mEglDisplay, config, mHolder, null);
  }

  private static final int EGL_OPENGL_ES2_BIT = 0x0004;
  private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
  private EGLConfig[] getConfigs()
  {
    final EGLConfig[] configs = new EGLConfig[40];
    final int[] numConfigs = new int[] { 0 };
    final int[] configAttributes = {
        EGL11.EGL_RED_SIZE, 5,
        EGL11.EGL_GREEN_SIZE, 6,
        EGL11.EGL_BLUE_SIZE, 5,
        EGL11.EGL_STENCIL_SIZE, 0,
        EGL11.EGL_DEPTH_SIZE, 16,
        EGL11.EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL11.EGL_NONE
      };

    egl.eglChooseConfig(
        mEglDisplay, configAttributes, configs, configs.length, numConfigs);

    final EGLConfig[] result = Arrays.copyOf(configs, numConfigs[0]);
    Arrays.sort(result, new ConfigSorter());

    return result;
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    // TODO should we pass something here?
  }

  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    mHolder = holder;
    nativeOnCreateSurface(mHolder.getSurface());
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    mHolder = null;
    nativeOnDestroySurface();
  }

  private void log(final String message)
  {
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG).show();
      }
    });

    Log.e("PTHREADRENDER", message);
  }

  private void _assert(boolean mustBeTrue, String message)
  {
    if (!mustBeTrue)
    {
      log(message);
      throw new RuntimeException(message);
    }
  }

  private int getEglError()
  {
    return egl.eglGetError();
  }

  private native void nativeOnCreateSurface(Surface surface);
  private native void nativeOnDestroySurface();
}
