package com.lody.virtual.tools.location;

/**
 *
 * @author 329716228@qq.com
 * @since 0.1.0
 *
 */
public class PositionConvertUtil {

    private static final double A = 6378245.0;
    private static final double PI = 3.1415926535897932384626;
    private static final double EE = 0.00669342162296594323;
    private static final double LON_BOUNDARY_MIN = 72.004;
    private static final double LAT_BOUNDARY_MIN = 0.8293;
    private static final double LON_BOUNDARY_MAX = 137.8347;
    private static final double LAT_BOUNDARY_MAX = 55.8271;
    private static final double X_PI = 3.14159265358979324 * 3000.0 / 180.0;

    /**
     * wgs84  to   gcj02
     * @param lat lat
     * @param lon lon
     * @return coordinate
     */
    public static CoordinateBean wgs84ToGcj02(double lat, double lon) {
        CoordinateBean info = new CoordinateBean();
        if (outOfChina(lat, lon)) {
            info.setChina(false);
            info.setLatitude(lat);
            info.setLongitude(lon);
        }else {
            double dLat = transformLat(lon - 105.0, lat - 35.0);
            double dLon = transformLon(lon - 105.0, lat - 35.0);
            double radLat = lat / 180.0 * PI;
            double magic = Math.sin(radLat);
            magic = 1 - EE * magic * magic;
            double sqrtMagic = Math.sqrt(magic);
            dLat = (dLat * 180.0) / ((A * (1 - EE)) / (magic * sqrtMagic) * PI);
            dLon = (dLon * 180.0) / (A / sqrtMagic * Math.cos(radLat) * PI);
            double mgLat = lat + dLat;
            double mgLon = lon + dLon;
            info.setChina(true);
            info.setLatitude(mgLat);
            info.setLongitude(mgLon);
        }
        return info;
    }

    /**
     * gcj02  to  wgs84
     * @param lat lat
     * @param lon lat
     * @return coordinate
     */
    public static CoordinateBean gcj02ToWgs84(double lat, double lon) {
        CoordinateBean info = new CoordinateBean();
        CoordinateBean gps = transform(lat, lon);
        double lontitude = lon * 2 - gps.getLongitude();
        double latitude = lat * 2 - gps.getLatitude();
        info.setChina(gps.isChina());
        info.setLatitude(latitude);
        info.setLongitude(lontitude);
        return info;
    }

    /**
     * 百度坐标系 (BD-09ll) 与 火星坐标系 (GCJ-02)的转换
     * 即 百度 转 谷歌中国、高德
     * @param bdLat bdLat
     * @param bdLon bdLon
     * @return coordinate
     */
    public CoordinateBean bd09tToGcj02(double bdLat,double bdLon){
        double x = bdLon - 0.0065;
        double y = bdLat - 0.006;
        double z = Math.sqrt(x * x + y * y) - 0.00002 * Math.sin(y * X_PI);
        double theta = Math.atan2(y, x) - 0.000003 * Math.cos(x * X_PI);
        double ggLng = z * Math.cos(theta);
        double ggLat = z * Math.sin(theta);
        return new CoordinateBean(ggLng,ggLat);
    }



    /**
     * 火星坐标系 (GCJ-02) 与百度坐标系 (BD-09ll) 的转换
     * 即谷歌中国、高德 转 百度
     * @param lon lng
     * @param lat lat
     * @return coordinate
     */
    public CoordinateBean gcj02ToBd09(double lat, double lon){
        double z = Math.sqrt(lon * lon + lat * lat) + 0.00002 * Math.sin(lat * X_PI);
        double theta = Math.atan2(lat, lon) + 0.000003 * Math.cos(lon * X_PI);
        double bdLng = z * Math.cos(theta) + 0.0065;
        double bdLat = z * Math.sin(theta) + 0.006;
        return new CoordinateBean(bdLng,bdLat);
    }

    /**
     * baidu09ll to wgs84
     * 即谷歌中国、高德 转 百度
     * @param lat lat
     * @param lon lng
     * @return coordinate
     */
    public CoordinateBean baidu09ToWgs84(double lat, double lon){
        CoordinateBean info = bd09tToGcj02(lat,lon);
        info = gcj02ToWgs84(info.getLatitude(),info.getLongitude());
        return info;
    }

    /**
     * 这里的传参是WGS84 即gps坐标
     * @param lat lat
     * @param lon lon
     * @return boolean
     */
    private static boolean outOfChina(double lat, double lon) {
        if (lon < LON_BOUNDARY_MIN || lon > LON_BOUNDARY_MAX){
            return true;
        }
        return lat < LAT_BOUNDARY_MIN || lat > LAT_BOUNDARY_MAX;
    }

    private static double transformLon(double x, double y) {
        double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1
                * Math.sqrt(Math.abs(x));
        ret += (20.0 * Math.sin(6.0 * x * PI) + 20.0 * Math.sin(2.0 * x * PI)) * 2.0 / 3.0;
        ret += (20.0 * Math.sin(x * PI) + 40.0 * Math.sin(x / 3.0 * PI)) * 2.0 / 3.0;
        ret += (150.0 * Math.sin(x / 12.0 * PI) + 300.0 * Math.sin(x / 30.0 * PI)) * 2.0 / 3.0;
        return ret;
    }

    private static double transformLat(double lng,double lat) {
        double ret = -100.0 + 2.0 * lng + 3.0 * lat + 0.2 * lat * lat + 0.1 * lng * lat + 0.2 * Math.sqrt(Math.abs(lng));
        ret += (20.0 * Math.sin(6.0 * lng * PI) + 20.0 * Math.sin(2.0 * lng * PI)) * 2.0 / 3.0;
        ret += (20.0 * Math.sin(lat * PI) + 40.0 * Math.sin(lat / 3.0 * PI)) * 2.0 / 3.0;
        ret += (160.0 * Math.sin(lat / 12.0 * PI) + 320 * Math.sin(lat * PI / 30.0)) * 2.0 / 3.0;
        return ret;
    }

    private static CoordinateBean transform(double lat, double lon) {
        CoordinateBean info = new CoordinateBean();
        if (outOfChina(lat, lon)) {
            info.setChina(false);
            info.setLatitude(lat);
            info.setLongitude(lon);
            return info;
        }
        double dLat = transformLat(lon - 105.0, lat - 35.0);
        double dLon = transformLon(lon - 105.0, lat - 35.0);
        double radLat = lat / 180.0 * PI;
        double magic = Math.sin(radLat);
        magic = 1 - EE * magic * magic;
        double sqrtMagic = Math.sqrt(magic);
        dLat = (dLat * 180.0) / ((A * (1 - EE)) / (magic * sqrtMagic) * PI);
        dLon = (dLon * 180.0) / (A / sqrtMagic * Math.cos(radLat) * PI);
        double mgLat = lat + dLat;
        double mgLon = lon + dLon;
        info.setChina(true);
        info.setLatitude(mgLat);
        info.setLongitude(mgLon);
        return info;
    }
}