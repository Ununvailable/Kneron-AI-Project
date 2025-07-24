class UsbHostBridge(private val usbDeviceConnection: UsbDeviceConnection) {
    fun bulkTransferOut(endpoint: Int, data: ByteArray, length: Int, timeout: Int): Int {
        return usbDeviceConnection.bulkTransfer(endpoint, data, 0, length, timeout)
    }

    fun bulkTransferIn(endpoint: Int, buffer: ByteArray, length: Int, transferred: IntArray, timeout: Int): Int {
        val result = usbDeviceConnection.bulkTransfer(endpoint, buffer, 0, length, timeout)
        transferred[0] = if (result >= 0) result else 0
        return result
    }

    fun controlTransfer(requestType: Int, request: Int, value: Int, index: Int, buffer: ByteArray, length: Int, timeout: Int): Int {
        return usbDeviceConnection.controlTransfer(requestType, request, value, index, buffer, 0, length, timeout)
    }
}
