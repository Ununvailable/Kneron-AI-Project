import 'dart:io';
import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'package:image/image.dart' as img;
import 'package:permission_handler/permission_handler.dart';
import 'kl520_ffi.dart';

void main() {
  runApp(const MaterialApp(home: ImageInspectorApp()));
}

class ImageInspectorApp extends StatefulWidget {
  const ImageInspectorApp({super.key});

  @override
  State<ImageInspectorApp> createState() => _ImageInspectorAppState();
}

class _ImageInspectorAppState extends State<ImageInspectorApp> {
  final List<File> _selectedImages = [];
  final List<String> _info = [];

  Future<void> _pickImages() async {
    final picker = ImagePicker();
    final permission = await Permission.photos.request();
    if (!permission.isGranted) return;

    final List<XFile>? pickedFiles = await picker.pickMultiImage();
    if (pickedFiles == null) return;

    _selectedImages.clear();
    _info.clear();

    for (var file in pickedFiles) {
      final f = File(file.path);
      final bytes = await f.readAsBytes();
      final decoded = img.decodeImage(bytes);

      if (decoded != null) {
        _selectedImages.add(f);
        _info.add("Format: ${decoded.format.name.toUpperCase()}, "
            "Dimensions: ${decoded.width}x${decoded.height}");
      } else {
        _info.add("Unsupported or corrupted image: ${file.path}");
      }
    }

    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Image Inspector")),
      body: Column(
        children: [
          ElevatedButton(
            onPressed: _pickImages,
            child: const Text("Pick Images"),
          ),
          const SizedBox(height: 10),
          Expanded(
            child: ListView.builder(
              itemCount: _selectedImages.length,
              itemBuilder: (context, index) {
                return ListTile(
                  title: Text(_info[index]),
                  trailing: ElevatedButton(
                    onPressed: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (_) =>
                              ImageDisplayScreen(file: _selectedImages[index]),
                        ),
                      );
                    },
                    child: const Text("View"),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}

class ImageDisplayScreen extends StatelessWidget {
  final File file;
  const ImageDisplayScreen({super.key, required this.file});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Image Viewer")),
      body: Center(
        child: Image.file(file),
      ),
    );
  }
}