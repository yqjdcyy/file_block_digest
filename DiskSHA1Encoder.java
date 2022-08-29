// 个人场景需指定 package
class DiskSHA1Encoder{

    static{
        // 加载 {java.library.path}/libDiskSHA1Encoder.so 文件
        System.loadLibrary("DiskSHA1Encoder");
    }

    private native String encode(String fileUrl);

    public static void main(String[] args){
        System.out.println(new DiskSHA1Encoder().encode(args[0]));
    }
}