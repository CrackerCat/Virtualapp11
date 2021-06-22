package com.lody.virtual.server.pm;

import android.content.ComponentName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.util.SparseArray;

import com.lody.virtual.client.core.VirtualCore;

import java.util.Map;

/**
 * @author Swift Gan
 * Create: 2019/4/28
 * Desc: Manager Conponent State
 * Process: Server
 */
public class ComponentStateManager {

    private static SparseArray<UserComponentState> helpers = new SparseArray<>();


    public static synchronized UserComponentState user(int userId) {
        UserComponentState state = helpers.get(userId);
        if (state == null) {
            state = new UserComponentState(userId);
            helpers.put(userId, state);
        }
        return state;
    }

    public static class UserComponentState {

        private SharedPreferences sharedPreferences;

        private UserComponentState(int userId) {
            sharedPreferences = VirtualCore.get().getContext().getSharedPreferences("va_components_state_u" + userId, Context.MODE_PRIVATE);
        }

        public int get(ComponentName componentName) {
            return sharedPreferences.getInt(componentKey(componentName), PackageManager.COMPONENT_ENABLED_STATE_DEFAULT);
        }

        public void set(ComponentName componentName, int state) {
            sharedPreferences.edit().putInt(componentKey(componentName), state).apply();
        }

        public void clear(String packageName) {
            Map<String,Integer> all = (Map<String, Integer>) sharedPreferences.getAll();
            if (all == null)
                return;
            for (String component:all.keySet()) {
                if (component.startsWith(packageName + "@")) {
                    sharedPreferences.edit().remove(component).apply();
                }
            }
        }

        public void clearAll() {
            sharedPreferences.edit().clear().apply();
        }

        private String componentKey(ComponentName componentName) {
            return componentName.getPackageName() + "@" + componentName.getClassName();
        }
    }

}
