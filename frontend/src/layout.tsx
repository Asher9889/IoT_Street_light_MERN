import { SidebarProvider, SidebarTrigger } from "@/components/ui/sidebar";
import { AppSidebar } from "./components/custom/AppSidebar";
import AppBreadcrumbs from "@/components/custom/Breadcrumbs";

export default function Layout({ children }: { children: React.ReactNode }) {
  return (
    <SidebarProvider>
      <div className="flex min-h-screen w-full">
        <AppSidebar />
        <main className="flex-1 p-4">
          <SidebarTrigger />
          <AppBreadcrumbs />
          {children}
        </main>
      </div>
    </SidebarProvider>
  );
}
