package com.lody.virtual.cpuabi;

import java.lang.reflect.Method;

public class RUtils
{
    public static Object getMethod(String jclazz, String jmethod)
    {
        jclazz = jclazz.replace("/",".");
        try{
            Class<?> clazz = Class.forName(jclazz);
            for(Method method : clazz.getDeclaredMethods())
            {
                if(method.getName().equals(jmethod))return method;
            }
        }catch (Exception e)
        {
            e.printStackTrace();
        }
        return null;
    }
}
