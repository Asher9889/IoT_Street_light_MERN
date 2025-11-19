// components/Breadcrumbs.tsx
import { useLocation, Link } from "react-router-dom";
import {
  Breadcrumb,
  BreadcrumbList,
  BreadcrumbItem,
  BreadcrumbLink,
  BreadcrumbSeparator,
  BreadcrumbPage,
} from "@/components/ui/breadcrumb";
import React from "react";

export default function AppBreadcrumbs() {
  const location = useLocation();

  // e.g. "/blogs/create" → ["blogs", "create"]
  const segments = location.pathname.split("/").filter(Boolean);

  // Build cumulative paths
  const paths = segments.map(
    (_, index) => "/" + segments.slice(0, index + 1).join("/")
  );

  return (
    <Breadcrumb className="mb-4">
      <BreadcrumbList>
        {/* Home */}
        <BreadcrumbItem>
          <BreadcrumbLink asChild>
            <Link to="/">Home</Link>
          </BreadcrumbLink>
        </BreadcrumbItem>

        {segments.map((segment, index) => {
          const label =
            segment.charAt(0).toUpperCase() +
            segment.slice(1).replace(/-/g, " ");
          const isLast = index === segments.length - 1;

          return (
            <React.Fragment key={paths[index]}>
              <BreadcrumbSeparator />

              <BreadcrumbItem>
                {isLast ? (
                  <BreadcrumbPage>{label}</BreadcrumbPage>
                ) : (
                  <BreadcrumbLink asChild>
                    <Link to={paths[index]}>{label}</Link>
                  </BreadcrumbLink>
                )}
              </BreadcrumbItem>
            </React.Fragment>
          );
        })}
      </BreadcrumbList>
    </Breadcrumb>
  );
}
